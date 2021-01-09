
struct GLFWwindow;

struct PkGraphicsWindow
{
    GLFWwindow* pWindow = nullptr;
    bool windowResized = false;
};

void PkGraphicsInitialise(PkGraphicsWindow& rGraphicsWindow);
void PkGraphicsCleanup();
void PkGraphicsRender();
