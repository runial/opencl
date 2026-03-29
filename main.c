// Vector addition with OpenCL
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include <CL/cl.h>

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

const size_t VECTOR_DIMENSIONS = 100000;
const size_t VERIFY_STEP = VECTOR_DIMENSIONS / 5;
const cl_ulong VECTOR_DIMENSIONS_UNSIGNED = VECTOR_DIMENSIONS;
const size_t VECTOR_SIZE = sizeof(float) * VECTOR_DIMENSIONS;
const cl_uint PLATFORM_SEARCH_LIMIT = 1;
const cl_uint DEVICE_SEARCH_LIMIT = 1;
const size_t PLATFORM_INFO_BUF_SIZE = 512;
const size_t DEVICE_INFO_BUF_SIZE = 512;
const float FLOAT_TOLERANCE = .00001;

const char *kernel_src = " \
__kernel void vadd( \
  __global float* vec_a, \
  __global float* vec_b, \
  __global float* vec_result, \
  ulong count \
) { \
  ulong i = get_global_id(0); \
  if (i < count) \
    vec_result[i] = vec_a[i] + vec_b[i]; \
} \
";

bool print_cl_platform_info(cl_platform_id platform) {
  bool ok = false;

  char *buf = malloc(sizeof(char) * PLATFORM_INFO_BUF_SIZE);
  if (buf == NULL)
    goto cleanup_buf;

  typedef struct {
    const char *user_friendly_name;
    cl_platform_info name;
  } PlatformParameter;

  // Currently only works with string parameters
  PlatformParameter params[] = {{"Platform", CL_PLATFORM_NAME},
                                {"Vendor", CL_PLATFORM_VENDOR},
                                {"CL Platform Version", CL_PLATFORM_VERSION}};
  size_t param_count = sizeof(params) / sizeof(PlatformParameter);

  printf("\nPLATFORM INFO:\n");

  // Fill buffer with appropriate values and print
  for (size_t i = 0; i < param_count; i++) {
    PlatformParameter param = params[i];

    // Find parameter size
    size_t param_size;
    cl_int status =
        clGetPlatformInfo(platform, param.name, 0, NULL, &param_size);
    if (status != CL_SUCCESS)
      goto cleanup_get_platform_info_query_size;
    if (param_size > PLATFORM_INFO_BUF_SIZE)
      goto cleanup_no_buf_space;

    // Write to buffer
    status = clGetPlatformInfo(platform, param.name, PLATFORM_INFO_BUF_SIZE,
                               buf, NULL);
    if (status != CL_SUCCESS)
      goto cleanup_get_platform_info;

    // Print to stdout
    printf("%s: %s\n", param.user_friendly_name, buf);
  }

  printf("\n");
  ok = true;

cleanup_get_platform_info:
cleanup_no_buf_space:
cleanup_get_platform_info_query_size:
  free(buf);
cleanup_buf:
  return ok;
}

bool print_cl_device_info(cl_device_id device) {
  bool ok = false;

  char *buf = malloc(sizeof(char) * DEVICE_INFO_BUF_SIZE);
  if (buf == NULL)
    goto cleanup_buf_alloc;

  typedef struct {
    const char *user_friendly_name;
    cl_device_info name;
  } DeviceParameter;

  DeviceParameter params[] = {{"Name", CL_DEVICE_NAME},
                              {"Vendor", CL_DEVICE_VENDOR},
                              {"CL Driver Version", CL_DRIVER_VERSION}};
  size_t param_count = sizeof(params) / sizeof(DeviceParameter);

  printf("\nDEVICE INFO:\n");

  for (size_t i = 0; i < param_count; i++) {
    DeviceParameter param = params[i];

    size_t param_size;
    cl_int status = clGetDeviceInfo(device, param.name, 0, NULL, &param_size);
    if (status != CL_SUCCESS)
      goto cleanup_get_device_info_query_size;
    if (param_size > DEVICE_INFO_BUF_SIZE)
      goto cleanup_no_buf_space;

    status =
        clGetDeviceInfo(device, param.name, DEVICE_INFO_BUF_SIZE, buf, NULL);
    if (status != CL_SUCCESS)
      goto cleanup_get_device_info;

    printf("%s: %s\n", param.user_friendly_name, buf);
  }

  printf("\n");
  ok = true;

cleanup_get_device_info:
cleanup_no_buf_space:
cleanup_get_device_info_query_size:
  free(buf);
cleanup_buf_alloc:
  return ok;
}

int main(void) {
  bool ok = false;

  // Initialize both vectors + result vector
  printf("Allocating memory for vectors...\n");
  float *vec_a = malloc(VECTOR_SIZE);
  if (vec_a == NULL)
    goto cleanup_vec_a;
  float *vec_b = malloc(VECTOR_SIZE);
  if (vec_b == NULL)
    goto cleanup_vec_b;
  float *vec_result = malloc(VECTOR_SIZE);
  if (vec_result == NULL)
    goto cleanup_vec_result;

  srand(time(NULL));
  for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
    vec_a[i] = rand() / (float)RAND_MAX;
    vec_b[i] = rand() / (float)RAND_MAX;
  }

  // Pick a device
  printf("Querying platforms...\n");
  cl_platform_id *platforms =
      malloc(sizeof(cl_platform_id) * PLATFORM_SEARCH_LIMIT);
  if (platforms == NULL)
    goto cleanup_platforms_alloc;
  cl_uint platform_count;

  cl_int status =
      clGetPlatformIDs(PLATFORM_SEARCH_LIMIT, platforms, &platform_count);
  if (status != CL_SUCCESS)
    goto cleanup_get_platform_ids;
  platform_count = MIN(platform_count, PLATFORM_SEARCH_LIMIT);
  printf("Found %u platform(s), defaulting to first found.\n", platform_count);
  if (platform_count == 0)
    goto cleanup_no_platforms;
  cl_platform_id platform = platforms[0];

  if (!print_cl_platform_info(platform))
    goto cleanup_print_platform_info;

  printf("Querying devices...\n");
  cl_device_id *devices = malloc(sizeof(cl_device_id) * DEVICE_SEARCH_LIMIT);
  if (devices == NULL)
    goto cleanup_devices_alloc;
  cl_uint device_count;

  status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, DEVICE_SEARCH_LIMIT,
                          devices, &device_count);
  if (status != CL_SUCCESS)
    goto cleanup_get_device_ids;
  device_count = MIN(device_count, DEVICE_SEARCH_LIMIT);
  printf("Found %u device(s), defaulting to first found.\n", device_count);
  if (device_count == 0)
    goto cleanup_no_devices;
  cl_device_id device = devices[0];

  if (!print_cl_device_info(device))
    goto cleanup_print_device_info;

  // Creates CL context
  // Note: Does NOT log async errors
  printf("Creating ctx...\n");
  cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
  if (status != CL_SUCCESS)
    goto cleanup_create_context;

  // Creates command queue
  printf("Creating command queue...\n");
  cl_command_queue queue =
      clCreateCommandQueueWithProperties(ctx, device, NULL, &status);
  if (status != CL_SUCCESS)
    goto cleanup_create_command_queue;

  // Creates and builds program + kernel from kernel_src
  printf("Building kernel...\n");
  cl_program program =
      clCreateProgramWithSource(ctx, 1, &kernel_src, NULL, &status);
  if (status != CL_SUCCESS)
    goto cleanup_create_program;
  status = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (status != CL_SUCCESS)
    goto cleanup_build_program;
  cl_kernel kernel = clCreateKernel(program, "vadd", &status);
  if (status != CL_SUCCESS)
    goto cleanup_create_kernel;

  // Allocates device memory
  printf("Allocating device memory...\n");
  cl_mem device_vec_a =
      clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, VECTOR_SIZE,
                     vec_a, &status);
  if (status != CL_SUCCESS)
    goto cleanup_device_vec_a;
  cl_mem device_vec_b =
      clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, VECTOR_SIZE,
                     vec_b, &status);
  if (status != CL_SUCCESS)
    goto cleanup_device_vec_b;
  cl_mem device_vec_result =
      clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, VECTOR_SIZE, NULL, &status);
  if (status != CL_SUCCESS)
    goto cleanup_device_vec_result;

  // Run kernel
  printf("Setting kernel arguments...\n");
  status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &device_vec_a);
  status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &device_vec_b);
  status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &device_vec_result);
  status |=
      clSetKernelArg(kernel, 3, sizeof(cl_ulong), &VECTOR_DIMENSIONS_UNSIGNED);
  if (status != CL_SUCCESS)
    goto cleanup_set_kernel_arg;
  printf("Enqueueing task...\n");
  cl_event kernel_vadd_task;
  status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &VECTOR_DIMENSIONS,
                                  NULL, 0, NULL, &kernel_vadd_task);
  if (status != CL_SUCCESS)
    goto cleanup_kernel_vadd_task_enqueue;
  status = clWaitForEvents(1, &kernel_vadd_task);
  if (status != CL_SUCCESS)
    goto cleanup_kernel_vadd_task_wait;

  // Read result
  printf("Reading result...\n");
  status = clEnqueueReadBuffer(queue, device_vec_result, CL_TRUE, 0,
                               VECTOR_SIZE, vec_result, 0, NULL, NULL);
  if (status != CL_SUCCESS)
    goto cleanup_kernel_read_result;

  // Verify result
  printf("Verifying results...\n");
  bool are_results_accurate = true;
  for (size_t i = 0; i < VECTOR_DIMENSIONS; i += VERIFY_STEP) {
    printf("Checking results (%zu/%zu)...\n", i, VECTOR_DIMENSIONS);
    for (size_t j = i; j < MIN(VECTOR_DIMENSIONS, i + VERIFY_STEP); j++) {
      float correct_result = vec_a[j] + vec_b[j];
      float calculated_result = vec_result[j];
      if (fabs(correct_result - calculated_result) > FLOAT_TOLERANCE) {
        are_results_accurate = false;
        fprintf(stderr, "Mismatch found: %f + %f = %f (correct answer: %f)\n",
                vec_a[j], vec_b[j], calculated_result, correct_result);
      }
    }
  }
  if (!are_results_accurate) {
    printf("Mismatches found, aborting...\n");
    goto cleanup_result_mismatch;
  }
  printf("All %zu results verified!\n", VECTOR_DIMENSIONS);

  // Cleanup
  ok = true;

cleanup_result_mismatch:
cleanup_kernel_read_result:
cleanup_kernel_vadd_task_wait:
  clReleaseEvent(kernel_vadd_task);
cleanup_kernel_vadd_task_enqueue:
cleanup_set_kernel_arg:
  clReleaseMemObject(device_vec_result);
cleanup_device_vec_result:
  clReleaseMemObject(device_vec_b);
cleanup_device_vec_b:
  clReleaseMemObject(device_vec_a);
cleanup_device_vec_a:
  clReleaseKernel(kernel);
cleanup_create_kernel:
cleanup_build_program:
  clReleaseProgram(program);
cleanup_create_program:
  clReleaseCommandQueue(queue);
cleanup_create_command_queue:
  clReleaseContext(ctx);
cleanup_create_context:
cleanup_print_device_info:
cleanup_no_devices:
cleanup_get_device_ids:
  free(devices);
cleanup_devices_alloc:
cleanup_print_platform_info:
cleanup_no_platforms:
cleanup_get_platform_ids:
  free(platforms);
cleanup_platforms_alloc:
  free(vec_result);
cleanup_vec_result:
  free(vec_b);
cleanup_vec_b:
  free(vec_a);
cleanup_vec_a:
  if (ok) {
    printf("Done.\n");
    return EXIT_SUCCESS;
  }
  fprintf(stderr, "Failed.\n");
  return EXIT_FAILURE;
}