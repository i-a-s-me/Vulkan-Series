// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include "sample.hpp"

class SampleApp
{
  static constexpr uint32_t SCREEN_WIDTH = 800;
  static constexpr uint32_t SCREEN_HEIGHT = 600;

  public:
  int launch(int argc, char *argv[]);

  ~SampleApp();

  private:
  GLFWwindow *m_window{};
  Box<SampleVulkan> m_vulkan{};
};

int main(int argc, char *argv[])
{
  SampleApp app;
  return app.launch(argc, argv);
}

int SampleApp::launch(int argc, char *argv[])
{
  if (!glfwInit())
  {
    printf("Failed to initialize GLFW\n");
    return -1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "i-a-s.me Vulkan Sample", nullptr, nullptr);
  if (!m_window)
  {
    printf("Failed to create GLFW window\n");
    return -1;
  }

  m_vulkan = Box<SampleVulkan>(new SampleVulkan());
  if (!m_vulkan->initialize([window = m_window](VkInstance instance, uint32_t &out_width, uint32_t &out_height,
                                                VkSurfaceKHR &out_surface) -> bool {
        if (glfwCreateWindowSurface(instance, window, nullptr, &out_surface) != VK_SUCCESS)
        {
          printf("Failed to create Vulkan surface\n");
          return false;
        }

        int32_t fb_width{}, fb_height{};
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        while ((fb_width == 0) || (fb_height == 0))
        {
          glfwWaitEvents();
          glfwGetFramebufferSize(window, &fb_width, &fb_height);
        }

        out_width = (uint32_t) fb_width;
        out_height = (uint32_t) fb_height;

        return true;
      }))
  {
    printf("Failed to initialize Vulkan\n");
    return -1;
  }

  glfwSetWindowUserPointer(m_window, m_vulkan.get());

  glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
    int32_t fb_width{}, fb_height{};
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    if ((fb_width == 0) || (fb_height == 0))
      return;
    static_cast<SampleVulkan *>(glfwGetWindowUserPointer(window))->resize(fb_width, fb_height);
  });

  while (!glfwWindowShouldClose(m_window))
  {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE))
      glfwSetWindowShouldClose(m_window, true);

    glfwPollEvents();

    // Don't render while the window is minimized (its framebuffer is 0x0 then)
    int32_t fb_width{}, fb_height{};
    glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
    if ((fb_width == 0) || (fb_height == 0))
    {
      glfwWaitEvents();
      continue;
    }

    if (!m_vulkan->render())
      break;
  }

  return 0;
}

SampleApp::~SampleApp()
{
  m_vulkan.reset();

  if (m_window)
    glfwDestroyWindow(m_window);

  glfwTerminate();
}
