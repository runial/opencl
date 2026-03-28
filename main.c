#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

const size_t VECTOR_DIMENSIONS = 1024;
const cl_uint PLATFORM_SEARCH_LIMIT = 1;
const cl_uint DEVICE_SEARCH_LIMIT = 1;
const size_t PLATFORM_INFO_BUF_SIZE = 512;
const size_t DEVICE_INFO_BUF_SIZE = 512;

const char *kernel_src = 
" \
__kernel void vadd( \
  __global float* vec_a, \
  __global float* vec_b, \
  __global float* vec_result, \
  unsigned count \
) { \
    int i = get_global_id(0); \
    if (i < count) \
      vec_result[i] = vec_a[i] + vec_b[i]; \
} \
";

bool print_cl_platform_info(cl_platform_id platform) {
  bool ok = false;

  char *buf = malloc(sizeof(char) * PLATFORM_INFO_BUF_SIZE);
  if (buf == NULL) goto cleanup_buf;

  typedef struct {
    const char *user_friendly_name;
    cl_platform_info name;
  } PlatformParameter;

  // Currently only works with string parameters
  PlatformParameter params[] = {
    {"Platform", CL_PLATFORM_NAME},
    {"Vendor", CL_PLATFORM_VENDOR},
    {"CL Platform Version", CL_PLATFORM_VERSION}
  };
  size_t param_count = sizeof(params) / sizeof(PlatformParameter);

  printf("\nPLATFORM INFO:\n");

  // Fill buffer with appropriate values and print
  for (size_t i = 0; i < param_count; i++) {
    PlatformParameter param = params[i];

    // Find parameter size
    size_t param_size;
    cl_int status =
      clGetPlatformInfo(platform, param.name, 0, NULL, &param_size);
    if (status != CL_SUCCESS) goto cleanup_get_platform_info_query_size;
    if (param_size > PLATFORM_INFO_BUF_SIZE) goto cleanup_no_buf_space;

    // Write to buffer
    status = clGetPlatformInfo(
      platform, param.name, PLATFORM_INFO_BUF_SIZE, buf, NULL);
    if (status != CL_SUCCESS) goto cleanup_get_platform_info;

    // Print to stdout
    printf("%s: %s\n", param.user_friendly_name, buf);
  }

  printf("\n");
  ok = true;

  cleanup_get_platform_info:
  cleanup_no_buf_space:
  cleanup_get_platform_info_query_size: free(buf);
  cleanup_buf: return ok;
}

bool print_cl_device_info(cl_device_id device) {
  bool ok = false;

  char *buf = malloc(sizeof(char) * DEVICE_INFO_BUF_SIZE);
  if (buf == NULL) goto cleanup_buf_alloc;

  typedef struct {
    const char *user_friendly_name;
    cl_device_info name;
  } DeviceParameter;

  DeviceParameter params[] = {
    {"Name", CL_DEVICE_NAME},
    {"Vendor", CL_DEVICE_VENDOR},
    {"CL Driver Version", CL_DRIVER_VERSION}
  };
  size_t param_count = sizeof(params) / sizeof(DeviceParameter);

  printf("\nDEVICE INFO:\n");

  for (size_t i = 0; i < param_count; i++) {
    DeviceParameter param = params[i];

    size_t param_size;
    cl_int status = clGetDeviceInfo(device, param.name, 0, NULL, &param_size);
    if (status != CL_SUCCESS) goto cleanup_get_device_info_query_size;
    if (param_size > DEVICE_INFO_BUF_SIZE) goto cleanup_no_buf_space;

    status = clGetDeviceInfo(
      device, param.name, DEVICE_INFO_BUF_SIZE, buf, NULL
    );
    if (status != CL_SUCCESS) goto cleanup_get_device_info;

    printf("%s: %s\n", param.user_friendly_name, buf);
  }

  printf("\n");
  ok = true;

  cleanup_get_device_info:
  cleanup_no_buf_space:
  cleanup_get_device_info_query_size: free(buf);
  cleanup_buf_alloc: return ok;
}

int main(void) {
  bool ok = false;

  // Initialize both vectors
  float *vec_a = malloc(sizeof(float) * VECTOR_DIMENSIONS);
  if (vec_a == NULL) goto cleanup_vec_a;
  float *vec_b = malloc(sizeof(float) * VECTOR_DIMENSIONS);
  if (vec_b == NULL) goto cleanup_vec_b;

  for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
    vec_a[i] = (float)rand() / RAND_MAX;
    vec_b[i] = (float)rand() / RAND_MAX;
  }

  // Pick a device
  cl_platform_id *platforms =
    malloc(sizeof(cl_platform_id) * PLATFORM_SEARCH_LIMIT);
  if (platforms == NULL) goto cleanup_platforms_alloc;
  cl_uint platform_count;

  cl_int status =
    clGetPlatformIDs(PLATFORM_SEARCH_LIMIT, platforms, &platform_count);
  if (status != CL_SUCCESS) goto cleanup_get_platform_ids;
  platform_count = MIN(platform_count, PLATFORM_SEARCH_LIMIT);
  printf("Found %u platform(s), defaulting to first found.\n", platform_count);
  if (platform_count == 0) goto cleanup_no_platforms;
  cl_platform_id platform = platforms[0];

  if (!print_cl_platform_info(platform)) goto cleanup_print_platform_info;

  cl_device_id *devices =
    malloc(sizeof(cl_device_id) * DEVICE_SEARCH_LIMIT);
  if (devices == NULL) goto cleanup_devices_alloc;
  cl_uint device_count;

  status = clGetDeviceIDs(
    platform,
    CL_DEVICE_TYPE_ALL,
    DEVICE_SEARCH_LIMIT,
    devices,
    &device_count
  );
  if (status != CL_SUCCESS) goto cleanup_get_device_ids;
  device_count = MIN(device_count, DEVICE_SEARCH_LIMIT);
  printf("Found %u device(s), defaulting to first found.\n", device_count);
  if (device_count == 0) goto cleanup_no_devices;
  cl_device_id device = devices[0];

  if (!print_cl_device_info(device)) goto cleanup_print_device_info;

  // Cleanup
  ok = true;

  cleanup_print_device_info:
  cleanup_no_devices:
  cleanup_get_device_ids: free(devices);
  cleanup_devices_alloc:
  cleanup_print_platform_info:
  cleanup_no_platforms:
  cleanup_get_platform_ids: free(platforms);
  cleanup_platforms_alloc: free(vec_b);
  cleanup_vec_b: free(vec_a);
  cleanup_vec_a: 
  if (ok) {
    printf("Done.\n");
    return EXIT_SUCCESS;
  }
  fprintf(stderr, "Failed.\n");
  return EXIT_FAILURE;
}