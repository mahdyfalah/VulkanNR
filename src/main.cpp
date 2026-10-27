// Minimal Vulkan compute host: c = a + b on the GPU, checked against the CPU.
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#define VK_CHECK(x) do { VkResult r_ = (x); if (r_ != VK_SUCCESS) { std::fprintf(stderr, "%s failed: %d\n", #x, r_); std::exit(1); } } while (0)

static std::vector<uint32_t> readSpirv(const char* path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) { std::fprintf(stderr, "cannot open %s\n", path); std::exit(1); }
  std::vector<uint32_t> code((size_t)file.tellg() / 4);
  file.seekg(0);
  file.read((char*)code.data(), code.size() * 4);
  return code;
}

struct Buffer { VkBuffer buffer; VkDeviceMemory memory; void* mapped; };

static uint32_t findMemory(VkPhysicalDevice gpu, uint32_t typeBits, VkMemoryPropertyFlags flags) {
  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(gpu, &props);
  for (uint32_t i = 0; i < props.memoryTypeCount; ++i)
    if ((typeBits & (1u << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags) return i;
  std::fprintf(stderr, "no suitable memory type\n");
  std::exit(1);
}

// Host-visible buffers keep the first experiment simple; device-local staging comes later.
static Buffer createBuffer(VkDevice device, VkPhysicalDevice gpu, VkDeviceSize size) {
  Buffer b{};
  VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  info.size = size;
  info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  VK_CHECK(vkCreateBuffer(device, &info, nullptr, &b.buffer));
  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device, b.buffer, &req);
  VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  alloc.allocationSize = req.size;
  alloc.memoryTypeIndex = findMemory(gpu, req.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VK_CHECK(vkAllocateMemory(device, &alloc, nullptr, &b.memory));
  VK_CHECK(vkBindBufferMemory(device, b.buffer, b.memory, 0));
  VK_CHECK(vkMapMemory(device, b.memory, 0, size, 0, &b.mapped));
  return b;
}

int main() {
  const uint32_t n = 1u << 20;
  const VkDeviceSize bytes = n * sizeof(float);

  VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app.apiVersion = VK_API_VERSION_1_3;
  VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instanceInfo.pApplicationInfo = &app;
  VkInstance instance;
  VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));

  uint32_t gpuCount = 0;
  vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
  if (!gpuCount) { std::fprintf(stderr, "no Vulkan device\n"); return 1; }
  std::vector<VkPhysicalDevice> gpus(gpuCount);
  vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
  VkPhysicalDevice gpu = gpus[0];
  VkPhysicalDeviceProperties gpuProps;
  vkGetPhysicalDeviceProperties(gpu, &gpuProps);
  std::printf("GPU: %s\n", gpuProps.deviceName);

  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> families(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());
  uint32_t family = UINT32_MAX;
  for (uint32_t i = 0; i < familyCount; ++i)
    if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { family = i; break; }
  if (family == UINT32_MAX) { std::fprintf(stderr, "no compute queue\n"); return 1; }

  float priority = 1.0f;
  VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueInfo.queueFamilyIndex = family;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priority;
  VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  VkDevice device;
  VK_CHECK(vkCreateDevice(gpu, &deviceInfo, nullptr, &device));
  VkQueue queue;
  vkGetDeviceQueue(device, family, 0, &queue);

  Buffer a = createBuffer(device, gpu, bytes), b = createBuffer(device, gpu, bytes), c = createBuffer(device, gpu, bytes);
  float* pa = (float*)a.mapped;
  float* pb = (float*)b.mapped;
  for (uint32_t i = 0; i < n; ++i) { pa[i] = (float)i * 0.5f; pb[i] = (float)(n - i); }

  std::vector<uint32_t> spirv = readSpirv(SHADER_DIR "/vector_add.spv");
  VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  moduleInfo.codeSize = spirv.size() * 4;
  moduleInfo.pCode = spirv.data();
  VkShaderModule module;
  VK_CHECK(vkCreateShaderModule(device, &moduleInfo, nullptr, &module));

  VkDescriptorSetLayoutBinding bindings[3]{};
  for (uint32_t i = 0; i < 3; ++i) {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }
  VkDescriptorSetLayoutCreateInfo setLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  setLayoutInfo.bindingCount = 3;
  setLayoutInfo.pBindings = bindings;
  VkDescriptorSetLayout setLayout;
  VK_CHECK(vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &setLayout));

  VkPushConstantRange pushRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t)};
  VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &setLayout;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushRange;
  VkPipelineLayout pipelineLayout;
  VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout));

  VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  pipelineInfo.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module, "main", nullptr};
  pipelineInfo.layout = pipelineLayout;
  VkPipeline pipeline;
  VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

  VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3};
  VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  VkDescriptorPool pool;
  VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool));
  VkDescriptorSetAllocateInfo setAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  setAlloc.descriptorPool = pool;
  setAlloc.descriptorSetCount = 1;
  setAlloc.pSetLayouts = &setLayout;
  VkDescriptorSet set;
  VK_CHECK(vkAllocateDescriptorSets(device, &setAlloc, &set));

  VkDescriptorBufferInfo bufferInfos[3] = {{a.buffer, 0, bytes}, {b.buffer, 0, bytes}, {c.buffer, 0, bytes}};
  VkWriteDescriptorSet writes[3]{};
  for (uint32_t i = 0; i < 3; ++i) {
    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = set;
    writes[i].dstBinding = i;
    writes[i].descriptorCount = 1;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[i].pBufferInfo = &bufferInfos[i];
  }
  vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

  VkCommandPoolCreateInfo cmdPoolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cmdPoolInfo.queueFamilyIndex = family;
  VkCommandPool cmdPool;
  VK_CHECK(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
  VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAlloc.commandPool = cmdPool;
  cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAlloc.commandBufferCount = 1;
  VkCommandBuffer cmd;
  VK_CHECK(vkAllocateCommandBuffers(device, &cmdAlloc, &cmd));

  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK(vkBeginCommandBuffer(cmd, &begin));
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &set, 0, nullptr);
  vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(n), &n);
  vkCmdDispatch(cmd, (n + 255) / 256, 1, 1);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cmd;
  VK_CHECK(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
  VK_CHECK(vkQueueWaitIdle(queue));

  const float* pc = (const float*)c.mapped;
  uint32_t wrong = 0;
  for (uint32_t i = 0; i < n; ++i)
    if (pc[i] != pa[i] + pb[i]) ++wrong;
  std::printf("vector_add n=%u: %s (%u mismatches)\n", n, wrong ? "FAIL" : "OK", wrong);

  vkDestroyCommandPool(device, cmdPool, nullptr);
  vkDestroyDescriptorPool(device, pool, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
  vkDestroyShaderModule(device, module, nullptr);
  for (Buffer* buf : {&a, &b, &c}) {
    vkUnmapMemory(device, buf->memory);
    vkDestroyBuffer(device, buf->buffer, nullptr);
    vkFreeMemory(device, buf->memory, nullptr);
  }
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
  return wrong ? 1 : 0;
}
