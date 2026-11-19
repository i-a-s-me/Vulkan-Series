// Released into the public domain.
// Written by I-A-S for use in i-a-s.me vulkan series.

#include <renderer.hpp>

class SampleApp
{
  static constexpr uint32_t SCREEN_WIDTH = 800;
  static constexpr uint32_t SCREEN_HEIGHT = 600;

  public:
  Result<int> launch(int argc, char *argv[]);

  ~SampleApp();

  private:
  GLFWwindow *m_window{};
  Box<Renderer> m_renderer{};
};

int main(int argc, char *argv[])
{
  SampleApp app;
  const auto result = app.launch(argc, argv);
  if (result.is_error())
  {
    printf("ERROR: %s\n", result.get_error().c_str());
    return -1;
  }
  return result.get_value();
}

Result<int> SampleApp::launch(int argc, char *argv[])
{
  if (!glfwInit())
    return "Failed to initialize GLFW";

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "i-a-s.me Vulkan Sample", nullptr, nullptr);
  if (!m_window)
    return "Failed to create GLFW window";

  CHECK_ASSIGN_RESULT(
      Renderer::create([window = m_window](VkInstance instance, uint32_t &out_width, uint32_t &out_height,
                                           VkSurfaceKHR &out_surface) -> Result<> {
        if (glfwCreateWindowSurface(instance, window, nullptr, &out_surface) != VK_SUCCESS)
          return "Failed to create Vulkan surface";

        int32_t fb_width{}, fb_height{};
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        while ((fb_width == 0) || (fb_height == 0))
        {
          glfwWaitEvents();
          glfwGetFramebufferSize(window, &fb_width, &fb_height);
        }

        out_width = (uint32_t) fb_width;
        out_height = (uint32_t) fb_height;

        return Ok();
      }),
      m_renderer);

  glfwSetWindowUserPointer(m_window, m_renderer.get());

  glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
    int32_t fb_width{}, fb_height{};
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    if ((fb_width == 0) || (fb_height == 0))
      return;
    static_cast<Renderer *>(glfwGetWindowUserPointer(window))->resize(fb_width, fb_height);
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

    CHECK_RESULT(m_renderer->render());
  }

  return 0;
}

SampleApp::~SampleApp()
{
  m_renderer.reset();

  if (m_window)
    glfwDestroyWindow(m_window);

  glfwTerminate();
}
