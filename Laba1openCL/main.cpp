#define JPEG_INTERNALS
#include "Image.hpp"
#include <iostream>
#include <OpenCL/OpenCL.h>
#include <fstream>
#include <string>

using namespace std;

const int LOCAL_WINDOW_SIZE = 16;

int super_mega_opencl();
int super_mega_opencl_color();

int main(int argc, const char * argv[]) {
  //super_mega_opencl();
  super_mega_opencl_color();
  return 0;
}

int* createMat(){
  int* Mat = new int [DCTSIZE2];
  for (int i = 0; i<DCTSIZE; i++){
    Mat[i] = (i+1);
    Mat[i*DCTSIZE] = (i+1);
  }
  for (int i = 1; i<DCTSIZE; i++){
    for (int j = 1; j<DCTSIZE; j++){
      Mat[i*DCTSIZE+j] = Mat[i]*Mat[j];
    }
  }
  return Mat;
}

int super_mega_opencl()
{
  printf("OpenCl information:\n");
  
  //Obtain the list of platforms available.
  cl_uint num_platforms(0);
  clGetPlatformIDs(0, nullptr, &num_platforms);
  cl_platform_id* platforms = new cl_platform_id[num_platforms];
  clGetPlatformIDs(num_platforms, platforms, nullptr);
  
  for (size_t i(0); i < num_platforms; ++i) {
    size_t param_value_size(0);
    clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, nullptr, &param_value_size);
    char *param_value = new char[param_value_size];
    clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, param_value_size, param_value, nullptr);
    printf("\tplatform %zu : %s\n", i, param_value);
    delete[] param_value;
    
    
    //Obtain the list of devices available on a platform[i].
    cl_uint num_devices(0);
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
    cl_device_id* devices = new cl_device_id[num_devices];
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices, devices, nullptr);
    
    
    //Get information about an OpenCL device.
    for (size_t i(0); i < num_devices; ++i) {
      size_t param_value_size(0);
      clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, nullptr, &param_value_size);
      char* param_name = new char[param_value_size];
      clGetDeviceInfo(devices[i], CL_DEVICE_NAME, param_value_size, param_name, nullptr);
      
      clGetDeviceInfo(devices[i], CL_DEVICE_TYPE, 0, nullptr, &param_value_size);
      cl_device_type* param_type = new cl_device_type[param_value_size];
      clGetDeviceInfo(devices[i], CL_DEVICE_TYPE, param_value_size, param_type, nullptr);
      std::string device_type = (*param_type == CL_DEVICE_TYPE_GPU ? "GPU" : "NOT_GPU");
      
      printf("\t\tdevice %zu: |%s| %s\n", i, device_type.c_str(), param_name);
    }
  }
  
  //Получим девайсы типа GPU на указанной платформе PLATFORM_SELECT и узнаем DEVICE_SELECT
  size_t PLATFORM_SELECT(0);
  cout<<"PLATFORM_SELECT: "<<PLATFORM_SELECT<<endl;
  //printf("\nEnter platform number: ");
  //scanf("%zu", &PLATFORM_SELECT);
  //cin>>PLATFORM_SELECT;
  size_t DEVICE_SELECT(1);
  cout<<"DEVICE_SELECT: "<<DEVICE_SELECT<<endl;
  //printf("\nEnter device number: ");
  //scanf("%zu", &DEVICE_SELECT);
  //cin>>DEVICE_SELECT;
  cl_uint num_devices(0);
  clGetDeviceIDs(platforms[PLATFORM_SELECT], CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
  cl_device_id* devices = new cl_device_id[num_devices];
  clGetDeviceIDs(platforms[PLATFORM_SELECT], CL_DEVICE_TYPE_ALL, num_devices, devices, nullptr);
  
  //Создание контекста на/из девайса(-ов) ------------------------------------------
  cl_context_properties properties[4] = {
    (cl_context_properties)CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[PLATFORM_SELECT],
    0, 0
  };
  cl_context context = clCreateContext(properties, 1, &devices[DEVICE_SELECT], nullptr, nullptr, nullptr);
  if (context == nullptr) {
    printf("Context doesn't created\n");
  }
  
  //Создание очереди   ----------------------------------------------------------------
  //порядок операции на девайсе
  
  cl_command_queue command_queue = clCreateCommandQueue(context, devices[DEVICE_SELECT], CL_QUEUE_PROFILING_ENABLE, nullptr);
  // ^ получим очеред без настроек (обыкновенную)
  
  if (command_queue == nullptr) {
    printf("Queue doesn't created\n");
  }
  
  //Загрузка программы на дивайс ------------------------------------------------------
  FILE *fp = fopen("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1openCL/Laba1openCL/program.cl", "rb");;
  if (fp == nullptr) {
    printf("File doesn't open\n");
    return 1;
  }
  fseek(fp, 0L, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  
  char* program_str = new char[file_size];
  fread(program_str, file_size, 1, fp);
  fclose(fp);
  //printf("File size %i \ntext: %s\n", file_size, program_str);
  
  cl_program program = clCreateProgramWithSource(context, 1, (const char **)&program_str, &file_size, nullptr);
  if (program == nullptr) {
    printf("Program doesn't read\n");
    return 2;
  }
  
  //Компилирование программы ----------------------------------------------------------
  if (clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr)) {
    size_t param_value_size(0);
    clGetProgramBuildInfo(program, devices[DEVICE_SELECT], CL_PROGRAM_BUILD_LOG, 0, nullptr, &param_value_size);
    char* param_value = new char[param_value_size];
    clGetProgramBuildInfo(program, devices[DEVICE_SELECT], CL_PROGRAM_BUILD_LOG, param_value_size, param_value, 0);
    printf("Error log: %s", param_value);
    return 3;
  }
  
  const char *kernel_name = "sum";
  cl_kernel kernel = clCreateKernel(program, kernel_name, nullptr);
  if (!kernel) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctKernel = clCreateKernel(program, "reapplicationJPEG", nullptr);
  if (!dctKernel) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel justDCTkernel = clCreateKernel(program, "justFloatDCT", nullptr);
  if (!justDCTkernel) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel divMat = clCreateKernel(program, "divMat", nullptr);
  if (!divMat) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctRigthEdge = clCreateKernel(program, "dctRigthEdge", nullptr);
  if (!dctRigthEdge) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctDownEdge = clCreateKernel(program, "dctDownEdge", nullptr);
  if (!dctRigthEdge) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctCorner = clCreateKernel(program, "dctCorner", nullptr);
  if (!dctRigthEdge) {
    printf("Kernel error\n");
    return 4;
  }
  
  
  Image img;
  img.readImage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/JPEG_example_JPG_RIP_010_gray.jpg");
  
  unsigned int rows = img.getRows();
  unsigned int cols = img.getCols();
  unsigned char *rawImg = img.getRawImg();
  
  static const double aanscalefactor[DCTSIZE] = {
    1.0, 1.387039845, 1.306562965, 1.175875602,
    1.0, 0.785694958, 0.541196100, 0.275899379
  };
  
  float  scaleMatQuant[DCTSIZE2 * 2];
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j] = 1.0/(aanscalefactor[i]*aanscalefactor[j]*8.0* img.getElemQuantTbl(i*DCTSIZE+j));
  
  
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j+ DCTSIZE2] = (aanscalefactor[i]*aanscalefactor[j]*0.125 * img.getElemQuantTbl(i*DCTSIZE+j));
  
  cl_mem buff_rawImg = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_uchar)*rows*cols, nullptr, nullptr);
  cl_mem buff_resDCT = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_short)*rows*cols, nullptr, nullptr);
  cl_mem buff_scaleMatQuant = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float)*DCTSIZE2*2, nullptr, nullptr);
  
  if (!buff_rawImg || !buff_resDCT || !buff_scaleMatQuant ) {
    printf("Alloc mem error\n");
    return 5;
  }
  
  short *matDCT = new short[rows*cols];
  unsigned int offset = 0;
  
  clEnqueueWriteBuffer(command_queue, buff_rawImg, false, 0, sizeof(cl_uchar)*rows*cols, rawImg, 0, nullptr, nullptr);
  clEnqueueWriteBuffer(command_queue, buff_resDCT, false, 0, sizeof(cl_short)*rows*cols, matDCT, 0, nullptr, nullptr);
  clEnqueueWriteBuffer(command_queue, buff_scaleMatQuant, false, 0, sizeof(cl_float)*DCTSIZE2*2, scaleMatQuant, 0, nullptr, nullptr);
  
  
  
  
  // Установка аругументов функции на девайсе  ----------------------------------------
  clSetKernelArg(dctKernel, 0, sizeof(cl_mem), &buff_rawImg);
  clSetKernelArg(dctKernel, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctKernel, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctKernel, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctKernel, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctKernel, 6, sizeof(cl_uint), &cols);
  
 
  short tmpZero = 0;
  clEnqueueFillBuffer(command_queue, buff_resDCT, &tmpZero, 1, 0, sizeof(short)*rows*cols, 0, nullptr, nullptr);
  ; // сколько тредов запускается
  cl_event kernel_event;
  for (int y_offset = 0; y_offset<= DCTSIZE; y_offset += DCTSIZE)
  {
    for (int x_offset = 0; x_offset<= DCTSIZE; x_offset += DCTSIZE)
    {
      int y_start = y_offset;
      int y_end = ((rows-y_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+y_offset;
      int x_start = x_offset;
      int x_end = ((cols-x_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+x_offset;
      offset = y_offset*cols + x_offset;
      clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
      size_t dct_local_work_size[2] = { DCTSIZE2, 1};
      size_t newRows = ((y_end-y_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[1];
      size_t newCols = ((x_end-x_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[0];
      size_t dct_global_work_size[2] = { newCols, newRows };
      cout << dct_global_work_size << endl;
      //cout<<"offset "<<offset<<" y_offset "<<y_offset<<" x_offset "<<x_offset<<endl<<"y_start "<<y_start<<" y_end "<<y_end<<endl<<"x_start "<<x_start<<" x_end "<<x_end<<endl<<"newRows "<<newRows<<" newCols "<<newCols<<endl<<"=================================="<<endl;
      
      if (clEnqueueNDRangeKernel(command_queue, dctKernel, 2, nullptr, dct_global_work_size, dct_local_work_size, 0, nullptr, &kernel_event)) {
        //^размерность потоков
        printf("Kernel execution failed\n");
        return 6;
      }
      cl_ulong startTime, endTime;
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, NULL);
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, NULL);
      double res = (endTime - startTime) ;
      printf("Kernel time program %f\n", res);
    }
  }
  
  clSetKernelArg(dctRigthEdge, 0, sizeof(cl_mem), &buff_rawImg);
  clSetKernelArg(dctRigthEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctRigthEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctRigthEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctRigthEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctRigthEdge, 6, sizeof(cl_uint), &cols);
  size_t dctRigthEdge_local_work_size[1] = { DCTSIZE };
  size_t dctRigthEdge_global_work_size[1] = { rows/2};
  offset = 0;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctRigthEdge_local_work_size[0] = DCTSIZE;
  dctRigthEdge_global_work_size[0] = (rows - LOCAL_WINDOW_SIZE)/2;
  offset = cols*DCTSIZE;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctDownEdge, 0, sizeof(cl_mem), &buff_rawImg);
  clSetKernelArg(dctDownEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctDownEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctDownEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctDownEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctDownEdge, 6, sizeof(cl_uint), &cols);
  size_t dctDownEdge_local_work_size[1] = { DCTSIZE };
  size_t dctDownEdge_global_work_size[1] = { cols/2 };
  offset = 0;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctDownEdge_local_work_size[0] = DCTSIZE;
  dctDownEdge_global_work_size[0] = (cols-LOCAL_WINDOW_SIZE)/2;
  offset = DCTSIZE;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctCorner, 0, sizeof(cl_mem), &buff_rawImg);
  clSetKernelArg(dctCorner, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctCorner, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctCorner, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctCorner, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  clSetKernelArg(dctCorner, 5, sizeof(cl_uint), &cols);
  size_t dctCorner_local_work_size[1] = { DCTSIZE };
  size_t dctCorner_global_work_size[1] = { DCTSIZE };
  if (clEnqueueNDRangeKernel(command_queue, dctCorner, 1, nullptr, dctCorner_global_work_size, dctCorner_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  //Копирование данных с девайса на хост ----------------------------------------------
  //clEnqueueReadBuffer(command_queue, buff_resDCT, true, 0, sizeof(cl_short)*rows*cols, matDCT, 0, nullptr, nullptr);
  
  int* mat = createMat();
  cl_mem buff_mat = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int)*DCTSIZE2, nullptr, nullptr);
  clEnqueueWriteBuffer(command_queue, buff_mat, false, 0, sizeof(cl_int)*DCTSIZE2, mat, 0, nullptr, nullptr);
  
  
  //cl_mem buff_resImg = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uchar)*rows*cols, nullptr, nullptr);
  //clEnqueueFillBuffer(command_queue, buff_resImg, &tmpZero, 1, 0, sizeof(cl_uchar)*rows*cols, 0, nullptr, nullptr);
  
  clSetKernelArg(divMat, 0, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(divMat, 1, sizeof(cl_mem), &buff_rawImg);
  clSetKernelArg(divMat, 2, sizeof(cl_uint), &rows);
  clSetKernelArg(divMat, 3, sizeof(cl_uint), &cols);
  
  
  size_t global_work_size[2] = { cols / DCTSIZE, rows };
  //cl_event kernel_event;
  
  if (clEnqueueNDRangeKernel(command_queue, divMat, 2, nullptr, global_work_size, nullptr, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution failed\n");
    return 6;
  }
  
  unsigned char *resImg = new unsigned char[rows * cols];
  
  clEnqueueReadBuffer(command_queue, buff_rawImg, true, 0, sizeof(cl_uchar)*rows*cols, resImg, 0, nullptr, nullptr);
  /*
   for (int i = 0; i<rows*cols; i++){
   matDCT[i] /= 64.0;
   }*/
  
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/JPEG_example_JPG_RIP_010_gray_OPENCL.pgm", resImg);
  /*
   cout<<"C: "<<endl;
   for (int i = 0; i<SIZE_X; i++)
   cout<<c[i]<<endl;*/
  /*
   clReleaseMemObject(buff_a);
   clReleaseMemObject(buff_b);
   clReleaseMemObject(buff_c);
   */
  delete[] matDCT;
  return 0;
}

int super_mega_opencl_color()
{
  printf("OpenCl information:\n");
  
  //Obtain the list of platforms available.
  cl_uint num_platforms(0);
  clGetPlatformIDs(0, nullptr, &num_platforms);
  cl_platform_id* platforms = new cl_platform_id[num_platforms];
  clGetPlatformIDs(num_platforms, platforms, nullptr);
  
  for (size_t i(0); i < num_platforms; ++i) {
    size_t param_value_size(0);
    clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, nullptr, &param_value_size);
    char *param_value = new char[param_value_size];
    clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, param_value_size, param_value, nullptr);
    printf("\tplatform %zu : %s\n", i, param_value);
    delete[] param_value;
    
    
    //Obtain the list of devices available on a platform[i].
    cl_uint num_devices(0);
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
    cl_device_id* devices = new cl_device_id[num_devices];
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices, devices, nullptr);
    
    
    //Get information about an OpenCL device.
    for (size_t i(0); i < num_devices; ++i) {
      size_t param_value_size(0);
      clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, nullptr, &param_value_size);
      char* param_name = new char[param_value_size];
      clGetDeviceInfo(devices[i], CL_DEVICE_NAME, param_value_size, param_name, nullptr);
      
      clGetDeviceInfo(devices[i], CL_DEVICE_TYPE, 0, nullptr, &param_value_size);
      cl_device_type* param_type = new cl_device_type[param_value_size];
      clGetDeviceInfo(devices[i], CL_DEVICE_TYPE, param_value_size, param_type, nullptr);
      std::string device_type = (*param_type == CL_DEVICE_TYPE_GPU ? "GPU" : "NOT_GPU");
      
      printf("\t\tdevice %zu: |%s| %s\n", i, device_type.c_str(), param_name);
    }
  }
  
  //Получим девайсы типа GPU на указанной платформе PLATFORM_SELECT и узнаем DEVICE_SELECT
  size_t PLATFORM_SELECT(0);
  cout<<"PLATFORM_SELECT: "<<PLATFORM_SELECT<<endl;
  //printf("\nEnter platform number: ");
  //scanf("%zu", &PLATFORM_SELECT);
  //cin>>PLATFORM_SELECT;
  size_t DEVICE_SELECT(1);
  cout<<"DEVICE_SELECT: "<<DEVICE_SELECT<<endl;
  //printf("\nEnter device number: ");
  //scanf("%zu", &DEVICE_SELECT);
  //cin>>DEVICE_SELECT;
  cl_uint num_devices(0);
  clGetDeviceIDs(platforms[PLATFORM_SELECT], CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
  cl_device_id* devices = new cl_device_id[num_devices];
  clGetDeviceIDs(platforms[PLATFORM_SELECT], CL_DEVICE_TYPE_ALL, num_devices, devices, nullptr);
  
  //Создание контекста на/из девайса(-ов) ------------------------------------------
  cl_context_properties properties[4] = {
    (cl_context_properties)CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[PLATFORM_SELECT],
    0, 0
  };
  cl_context context = clCreateContext(properties, 1, &devices[DEVICE_SELECT], nullptr, nullptr, nullptr);
  if (context == nullptr) {
    printf("Context doesn't created\n");
  }
  
  //Создание очереди   ----------------------------------------------------------------
  //порядок операции на девайсе
  
  cl_command_queue command_queue = clCreateCommandQueue(context, devices[DEVICE_SELECT], CL_QUEUE_PROFILING_ENABLE, nullptr);
  // ^ получим очеред без настроек (обыкновенную)
  
  if (command_queue == nullptr) {
    printf("Queue doesn't created\n");
  }
  
  //Загрузка программы на дивайс ------------------------------------------------------
  FILE *fp = fopen("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1openCL/Laba1openCL/program.cl", "rb");;
  if (fp == nullptr) {
    printf("File doesn't open\n");
    return 1;
  }
  fseek(fp, 0L, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  
  char* program_str = new char[file_size];
  fread(program_str, file_size, 1, fp);
  fclose(fp);
  //printf("File size %i \ntext: %s\n", file_size, program_str);
  
  cl_program program = clCreateProgramWithSource(context, 1, (const char **)&program_str, &file_size, nullptr);
  if (program == nullptr) {
    printf("Program doesn't read\n");
    return 2;
  }
  
  //Компилирование программы ----------------------------------------------------------
  if (clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr)) {
    size_t param_value_size(0);
    clGetProgramBuildInfo(program, devices[DEVICE_SELECT], CL_PROGRAM_BUILD_LOG, 0, nullptr, &param_value_size);
    char* param_value = new char[param_value_size];
    clGetProgramBuildInfo(program, devices[DEVICE_SELECT], CL_PROGRAM_BUILD_LOG, param_value_size, param_value, 0);
    printf("Error log: %s", param_value);
    return 3;
  }
  
  const char *kernel_name = "sum";
  cl_kernel kernel = clCreateKernel(program, kernel_name, nullptr);
  if (!kernel) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctKernel = clCreateKernel(program, "reapplicationJPEG", nullptr);
  if (!dctKernel) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel justDCTkernel = clCreateKernel(program, "justFloatDCT", nullptr);
  if (!justDCTkernel) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel divMat = clCreateKernel(program, "divMat", nullptr);
  if (!divMat) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctRigthEdge = clCreateKernel(program, "dctRigthEdge", nullptr);
  if (!dctRigthEdge) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctDownEdge = clCreateKernel(program, "dctDownEdge", nullptr);
  if (!dctRigthEdge) {
    printf("Kernel error\n");
    return 4;
  }
  
  cl_kernel dctCorner = clCreateKernel(program, "dctCorner", nullptr);
  if (!dctRigthEdge) {
    printf("Kernel error\n");
    return 4;
  }
  
  
  Image img;
  img.readColorImage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/wallpapersden.jpg");
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/wallpapersdenY.pgm", img.getYRawImg());
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/wallpapersdenCb.pgm", img.getCbRawImg());
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/wallpapersdenCr.pgm", img.getCrRawImg());
  
  unsigned int rows = img.getRows();
  unsigned int cols = img.getCols();
  unsigned char *rawYImg = img.getYRawImg();
  
  static const double aanscalefactor[DCTSIZE] = {
    1.0, 1.387039845, 1.306562965, 1.175875602,
    1.0, 0.785694958, 0.541196100, 0.275899379
  };
  
  float  scaleMatQuant[DCTSIZE2 * 2];
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j] = 1.0/(aanscalefactor[i]*aanscalefactor[j]*8.0* img.getElemQuantTbl(i*DCTSIZE+j));
  
  
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j+ DCTSIZE2] = (aanscalefactor[i]*aanscalefactor[j]*0.125 * img.getElemQuantTbl(i*DCTSIZE+j));
  
  cl_mem buff_rawYImg = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_uchar)*rows*cols, nullptr, nullptr);
  cl_mem buff_resDCT = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_short)*rows*cols, nullptr, nullptr);
  cl_mem buff_scaleMatQuant = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float)*DCTSIZE2*2, nullptr, nullptr);
  
  if (!buff_rawYImg || !buff_resDCT || !buff_scaleMatQuant ) {
    printf("Alloc mem error\n");
    return 5;
  }
  
  unsigned int offset = 0;
  
  clEnqueueWriteBuffer(command_queue, buff_rawYImg, false, 0, sizeof(cl_uchar)*rows*cols, rawYImg, 0, nullptr, nullptr);
  clEnqueueWriteBuffer(command_queue, buff_scaleMatQuant, false, 0, sizeof(cl_float)*DCTSIZE2*2, scaleMatQuant, 0, nullptr, nullptr);
  
  
  
  
  // Установка аругументов функции на девайсе  ----------------------------------------
  clSetKernelArg(dctKernel, 0, sizeof(cl_mem), &buff_rawYImg);
  clSetKernelArg(dctKernel, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctKernel, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctKernel, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctKernel, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctKernel, 6, sizeof(cl_uint), &cols);
  
  
  short tmpZero = 0;
  clEnqueueFillBuffer(command_queue, buff_resDCT, &tmpZero, 1, 0, sizeof(short)*rows*cols, 0, nullptr, nullptr);
  
  cl_event kernel_event;
  for (int y_offset = 0; y_offset<= DCTSIZE; y_offset += DCTSIZE)
  {
    for (int x_offset = 0; x_offset<= DCTSIZE; x_offset += DCTSIZE)
    {
      int y_start = y_offset;
      int y_end = ((rows-y_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+y_offset;
      int x_start = x_offset;
      int x_end = ((cols-x_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+x_offset;
      offset = y_offset*cols + x_offset;
      clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
      size_t dct_local_work_size[2] = { DCTSIZE2, 1};
      size_t newRows = ((y_end-y_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[1];
      size_t newCols = ((x_end-x_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[0];
      size_t dct_global_work_size[2] = { newCols, newRows };
      cout << dct_global_work_size << endl;
      //cout<<"offset "<<offset<<" y_offset "<<y_offset<<" x_offset "<<x_offset<<endl<<"y_start "<<y_start<<" y_end "<<y_end<<endl<<"x_start "<<x_start<<" x_end "<<x_end<<endl<<"newRows "<<newRows<<" newCols "<<newCols<<endl<<"=================================="<<endl;
      
      if (clEnqueueNDRangeKernel(command_queue, dctKernel, 2, nullptr, dct_global_work_size, dct_local_work_size, 0, nullptr, &kernel_event)) {
        //^размерность потоков
        printf("Kernel execution failed\n");
        return 6;
      }
      cl_ulong startTime, endTime;
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, NULL);
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, NULL);
      double res = (endTime - startTime) ;
      printf("Kernel time program %f\n", res);
    }
  }
  
  clSetKernelArg(dctRigthEdge, 0, sizeof(cl_mem), &buff_rawYImg);
  clSetKernelArg(dctRigthEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctRigthEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctRigthEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctRigthEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctRigthEdge, 6, sizeof(cl_uint), &cols);
  size_t dctRigthEdge_local_work_size[1] = { DCTSIZE };
  size_t dctRigthEdge_global_work_size[1] = { rows/2};
  offset = 0;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctRigthEdge_local_work_size[0] = DCTSIZE;
  dctRigthEdge_global_work_size[0] = (rows - LOCAL_WINDOW_SIZE)/2;
  offset = cols*DCTSIZE;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctDownEdge, 0, sizeof(cl_mem), &buff_rawYImg);
  clSetKernelArg(dctDownEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctDownEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctDownEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctDownEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctDownEdge, 6, sizeof(cl_uint), &cols);
  size_t dctDownEdge_local_work_size[1] = { DCTSIZE };
  size_t dctDownEdge_global_work_size[1] = { cols/2 };
  offset = 0;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctDownEdge_local_work_size[0] = DCTSIZE;
  dctDownEdge_global_work_size[0] = (cols-LOCAL_WINDOW_SIZE)/2;
  offset = DCTSIZE;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctCorner, 0, sizeof(cl_mem), &buff_rawYImg);
  clSetKernelArg(dctCorner, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctCorner, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctCorner, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctCorner, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  clSetKernelArg(dctCorner, 5, sizeof(cl_uint), &cols);
  size_t dctCorner_local_work_size[1] = { DCTSIZE };
  size_t dctCorner_global_work_size[1] = { DCTSIZE };
  if (clEnqueueNDRangeKernel(command_queue, dctCorner, 1, nullptr, dctCorner_global_work_size, dctCorner_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  //Копирование данных с девайса на хост ----------------------------------------------
  //clEnqueueReadBuffer(command_queue, buff_resDCT, true, 0, sizeof(cl_short)*rows*cols, matDCT, 0, nullptr, nullptr);
  
  
  //cl_mem buff_resImg = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uchar)*rows*cols, nullptr, nullptr);
  //clEnqueueFillBuffer(command_queue, buff_resImg, &tmpZero, 1, 0, sizeof(cl_uchar)*rows*cols, 0, nullptr, nullptr);
  
  clSetKernelArg(divMat, 0, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(divMat, 1, sizeof(cl_mem), &buff_rawYImg);
  clSetKernelArg(divMat, 2, sizeof(cl_uint), &rows);
  clSetKernelArg(divMat, 3, sizeof(cl_uint), &cols);
  
  
  size_t global_work_size[2] = { cols / DCTSIZE, rows };
  //cl_event kernel_event;
  
  if (clEnqueueNDRangeKernel(command_queue, divMat, 2, nullptr, global_work_size, nullptr, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution failed\n");
    return 6;
  }
  
  unsigned char *resYImg = new unsigned char[rows * cols];
  
  clEnqueueReadBuffer(command_queue, buff_rawYImg, true, 0, sizeof(cl_uchar)*rows*cols, resYImg, 0, nullptr, nullptr);
  
  img.setYRawImg(resYImg);
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/DCT_Y.pgm", resYImg);
  
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  
  unsigned char *rawCbImg = img.getCbRawImg();
  
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j] = 1.0/(aanscalefactor[i]*aanscalefactor[j]*8.0* img.getElemQuantTblTwo(i*DCTSIZE+j));
  
  
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j+ DCTSIZE2] = (aanscalefactor[i]*aanscalefactor[j]*0.125 * img.getElemQuantTblTwo(i*DCTSIZE+j));
  
  cl_mem buff_rawCbImg = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_uchar)*rows*cols, nullptr, nullptr);
  
  if (!buff_rawCbImg) {
    printf("Alloc mem error\n");
    return 5;
  }
  
  
  offset = 0;
  
  clEnqueueWriteBuffer(command_queue, buff_rawCbImg, false, 0, sizeof(cl_uchar)*rows*cols, rawCbImg, 0, nullptr, nullptr);
  clEnqueueWriteBuffer(command_queue, buff_scaleMatQuant, false, 0, sizeof(cl_float)*DCTSIZE2*2, scaleMatQuant, 0, nullptr, nullptr);
  clEnqueueFillBuffer(command_queue, buff_resDCT, &tmpZero, 1, 0, sizeof(short)*rows*cols, 0, nullptr, nullptr);
  
  
  
  
  // Установка аругументов функции на девайсе  ----------------------------------------
  clSetKernelArg(dctKernel, 0, sizeof(cl_mem), &buff_rawCbImg);
  clSetKernelArg(dctKernel, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctKernel, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctKernel, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctKernel, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctKernel, 6, sizeof(cl_uint), &cols);
  
  
  

  
  for (int y_offset = 0; y_offset<= DCTSIZE; y_offset += DCTSIZE)
  {
    for (int x_offset = 0; x_offset<= DCTSIZE; x_offset += DCTSIZE)
    {
      int y_start = y_offset;
      int y_end = ((rows-y_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+y_offset;
      int x_start = x_offset;
      int x_end = ((cols-x_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+x_offset;
      offset = y_offset*cols + x_offset;
      clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
      size_t dct_local_work_size[2] = { DCTSIZE2, 1};
      size_t newRows = ((y_end-y_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[1];
      size_t newCols = ((x_end-x_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[0];
      size_t dct_global_work_size[2] = { newCols, newRows };
      cout << dct_global_work_size << endl;
      
      if (clEnqueueNDRangeKernel(command_queue, dctKernel, 2, nullptr, dct_global_work_size, dct_local_work_size, 0, nullptr, &kernel_event)) {
        //^размерность потоков
        printf("Kernel execution failed\n");
        return 6;
      }
      cl_ulong startTime, endTime;
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, NULL);
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, NULL);
      double res = (endTime - startTime) ;
      printf("Kernel time program %f\n", res);
    }
  }
  
  clSetKernelArg(dctRigthEdge, 0, sizeof(cl_mem), &buff_rawCbImg);
  clSetKernelArg(dctRigthEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctRigthEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctRigthEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctRigthEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctRigthEdge, 6, sizeof(cl_uint), &cols);
  dctRigthEdge_local_work_size[0] = { DCTSIZE };
  dctRigthEdge_global_work_size[0] = { rows/2};
  offset = 0;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctRigthEdge_local_work_size[0] = DCTSIZE;
  dctRigthEdge_global_work_size[0] = (rows - LOCAL_WINDOW_SIZE)/2;
  offset = cols*DCTSIZE;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctDownEdge, 0, sizeof(cl_mem), &buff_rawCbImg);
  clSetKernelArg(dctDownEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctDownEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctDownEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctDownEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctDownEdge, 6, sizeof(cl_uint), &cols);
  dctDownEdge_local_work_size[0] = { DCTSIZE };
  dctDownEdge_global_work_size[0] = { cols/2 };
  offset = 0;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctDownEdge_local_work_size[0] = DCTSIZE;
  dctDownEdge_global_work_size[0] = (cols-LOCAL_WINDOW_SIZE)/2;
  offset = DCTSIZE;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctCorner, 0, sizeof(cl_mem), &buff_rawCbImg);
  clSetKernelArg(dctCorner, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctCorner, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctCorner, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctCorner, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  clSetKernelArg(dctCorner, 5, sizeof(cl_uint), &cols);
  dctCorner_local_work_size[0] = { DCTSIZE };
  dctCorner_global_work_size[0] = { DCTSIZE };
  if (clEnqueueNDRangeKernel(command_queue, dctCorner, 1, nullptr, dctCorner_global_work_size, dctCorner_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  //Копирование данных с девайса на хост ----------------------------------------------
  
  clSetKernelArg(divMat, 0, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(divMat, 1, sizeof(cl_mem), &buff_rawCbImg);
  clSetKernelArg(divMat, 2, sizeof(cl_uint), &rows);
  clSetKernelArg(divMat, 3, sizeof(cl_uint), &cols);
  
  
  global_work_size[0] =  cols / DCTSIZE;
  global_work_size[1] = rows;
  
  if (clEnqueueNDRangeKernel(command_queue, divMat, 2, nullptr, global_work_size, nullptr, 0, nullptr, &kernel_event)) {
    printf("Kernel execution failed\n");
    return 6;
  }
  
  unsigned char *resCbImg = new unsigned char[rows * cols];
  
  clEnqueueReadBuffer(command_queue, buff_rawCbImg, true, 0, sizeof(cl_uchar)*rows*cols, resCbImg, 0, nullptr, nullptr);
  
  img.setCbRawImg(resCbImg);
  
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/DCT_Cb.pgm", resCbImg);
  
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  
  unsigned char *rawCrImg = img.getCrRawImg();
  
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j] = 1.0/(aanscalefactor[i]*aanscalefactor[j]*8.0* img.getElemQuantTblThree(i*DCTSIZE+j));
  
  
  for (int i = 0; i<DCTSIZE; i++)
    for (int j = 0; j<DCTSIZE; j++)
      scaleMatQuant[i*DCTSIZE+j+ DCTSIZE2] = (aanscalefactor[i]*aanscalefactor[j]*0.125 * img.getElemQuantTblThree(i*DCTSIZE+j));
  
  cl_mem buff_rawCrImg = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_uchar)*rows*cols, nullptr, nullptr);
  
  if (!buff_rawCrImg || !buff_resDCT || !buff_scaleMatQuant ) {
    printf("Alloc mem error\n");
    return 5;
  }
  
  
  offset = 0;
  
  clEnqueueWriteBuffer(command_queue, buff_rawCrImg, false, 0, sizeof(cl_uchar)*rows*cols, rawCrImg, 0, nullptr, nullptr);
  clEnqueueWriteBuffer(command_queue, buff_scaleMatQuant, false, 0, sizeof(cl_float)*DCTSIZE2*2, scaleMatQuant, 0, nullptr, nullptr);
  
  
  
  
  // Установка аругументов функции на девайсе  ----------------------------------------
  clSetKernelArg(dctKernel, 0, sizeof(cl_mem), &buff_rawCrImg);
  clSetKernelArg(dctKernel, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctKernel, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctKernel, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctKernel, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctKernel, 6, sizeof(cl_uint), &cols);
  
  
  clEnqueueFillBuffer(command_queue, buff_resDCT, &tmpZero, 1, 0, sizeof(short)*rows*cols, 0, nullptr, nullptr);
  
  
  for (int y_offset = 0; y_offset<= DCTSIZE; y_offset += DCTSIZE)
  {
    for (int x_offset = 0; x_offset<= DCTSIZE; x_offset += DCTSIZE)
    {
      int y_start = y_offset;
      int y_end = ((rows-y_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+y_offset;
      int x_start = x_offset;
      int x_end = ((cols-x_offset)/LOCAL_WINDOW_SIZE)*LOCAL_WINDOW_SIZE+x_offset;
      offset = y_offset*cols + x_offset;
      clSetKernelArg(dctKernel, 5, sizeof(cl_uint), &offset);
      size_t dct_local_work_size[2] = { DCTSIZE2, 1};
      size_t newRows = ((y_end-y_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[1];
      size_t newCols = ((x_end-x_start)/LOCAL_WINDOW_SIZE)*dct_local_work_size[0];
      size_t dct_global_work_size[2] = { newCols, newRows };
      cout << dct_global_work_size << endl;
      
      if (clEnqueueNDRangeKernel(command_queue, dctKernel, 2, nullptr, dct_global_work_size, dct_local_work_size, 0, nullptr, &kernel_event)) {
        //^размерность потоков
        printf("Kernel execution failed\n");
        return 6;
      }
      cl_ulong startTime, endTime;
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(startTime), &startTime, NULL);
      clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(endTime), &endTime, NULL);
      double res = (endTime - startTime) ;
      printf("Kernel time program %f\n", res);
    }
  }
  
  clSetKernelArg(dctRigthEdge, 0, sizeof(cl_mem), &buff_rawCrImg);
  clSetKernelArg(dctRigthEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctRigthEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctRigthEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctRigthEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctRigthEdge, 6, sizeof(cl_uint), &cols);
  dctRigthEdge_local_work_size[0] = { DCTSIZE };
  dctRigthEdge_global_work_size[0] = { rows/2};
  offset = 0;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctRigthEdge_local_work_size[0] = DCTSIZE;
  dctRigthEdge_global_work_size[0] = (rows - LOCAL_WINDOW_SIZE)/2;
  offset = cols*DCTSIZE;
  clSetKernelArg(dctRigthEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctRigthEdge, 1, nullptr, dctRigthEdge_global_work_size, dctRigthEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctDownEdge, 0, sizeof(cl_mem), &buff_rawCrImg);
  clSetKernelArg(dctDownEdge, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctDownEdge, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctDownEdge, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctDownEdge, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  //clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  clSetKernelArg(dctDownEdge, 6, sizeof(cl_uint), &cols);
  dctDownEdge_local_work_size[0] = { DCTSIZE };
  dctDownEdge_global_work_size[0] = { cols/2 };
  offset = 0;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  dctDownEdge_local_work_size[0] = DCTSIZE;
  dctDownEdge_global_work_size[0] = (cols-LOCAL_WINDOW_SIZE)/2;
  offset = DCTSIZE;
  clSetKernelArg(dctDownEdge, 5, sizeof(cl_uint), &offset);
  if (clEnqueueNDRangeKernel(command_queue, dctDownEdge, 1, nullptr, dctDownEdge_global_work_size, dctDownEdge_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  clSetKernelArg(dctCorner, 0, sizeof(cl_mem), &buff_rawCrImg);
  clSetKernelArg(dctCorner, 1, sizeof(cl_uint), &rows);
  clSetKernelArg(dctCorner, 2, sizeof(cl_uint), &cols);
  clSetKernelArg(dctCorner, 3, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(dctCorner, 4, sizeof(cl_mem), &buff_scaleMatQuant);
  clSetKernelArg(dctCorner, 5, sizeof(cl_uint), &cols);
  dctCorner_local_work_size[0] = { DCTSIZE };
  dctCorner_global_work_size[0] = { DCTSIZE };
  if (clEnqueueNDRangeKernel(command_queue, dctCorner, 1, nullptr, dctCorner_global_work_size, dctCorner_local_work_size, 0, nullptr, &kernel_event)) {
    //^размерность потоков
    printf("Kernel execution FAILED\n");
    return 6;
  }
  
  //Копирование данных с девайса на хост ----------------------------------------------
  
  clSetKernelArg(divMat, 0, sizeof(cl_mem), &buff_resDCT);
  clSetKernelArg(divMat, 1, sizeof(cl_mem), &buff_rawCrImg);
  clSetKernelArg(divMat, 2, sizeof(cl_uint), &rows);
  clSetKernelArg(divMat, 3, sizeof(cl_uint), &cols);
  
  
  global_work_size[0] =  cols / DCTSIZE;
  global_work_size[1] = rows;
  
  if (clEnqueueNDRangeKernel(command_queue, divMat, 2, nullptr, global_work_size, nullptr, 0, nullptr, &kernel_event)) {
    printf("Kernel execution failed\n");
    return 6;
  }
  
  unsigned char *resCrImg = new unsigned char[rows * cols];
  
  clEnqueueReadBuffer(command_queue, buff_rawCrImg, true, 0, sizeof(cl_uchar)*rows*cols, resCrImg, 0, nullptr, nullptr);
  
  img.setCrRawImg(resCrImg);
  
  img.writePGMimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/DCT_Cr.pgm", resCrImg);
  
  img.writeJPEGimage("/Users/ilkin_galoev/Documents/7 semester/Parallel Programing 2/Laba1/Laba1/Color_RES_JPEG.jpg");
  
  /*
   clReleaseMemObject(buff_a);
   clReleaseMemObject(buff_b);
   clReleaseMemObject(buff_c);
   */
  return 0;
}
