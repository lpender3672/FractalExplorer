#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <complex>
#include <math.h>
#include <fstream>

//Constants
static const int target_fps = 60;
static const int sample_rate = 48000;
static const int max_freq = 4000;
static const int window_w_init = 1920;
static const int window_h_init = 1080;
static const int starting_fractal = 8;
static const int max_iters = 200;
static const double escape_radius_sq = 1000.0;
static const char window_name[] = "Double Pendulum Fractal";

//Settings
static int window_w = window_w_init;
static int window_h = window_h_init;
static double cam_x = 0.0;
static double cam_y = 0.0;
static double cam_zoom = 200.0;
static int cam_x_fp = 0;
static int cam_y_fp = 0;
static double cam_x_dest = cam_x;
static double cam_y_dest = cam_y;
static double cam_zoom_dest = cam_zoom;
static bool sustain = true;
static bool normalized = true;
static bool use_color = false;
static bool hide_orbit = true;
static double jx = 1e8;
static double jy = 1e8;
static int frame = 0;
static int animation_frame = 1;

//Fractal abstraction definition
typedef void (*Fractal)(double&, double&, double, double);
static Fractal fractal = nullptr;

//Blend modes
const sf::BlendMode BlendAlpha(sf::BlendMode::SrcAlpha, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add,
                               sf::BlendMode::Zero, sf::BlendMode::One, sf::BlendMode::Add);
const sf::BlendMode BlendIgnoreAlpha(sf::BlendMode::One, sf::BlendMode::Zero, sf::BlendMode::Add,
                                     sf::BlendMode::Zero, sf::BlendMode::One, sf::BlendMode::Add);

//Screen utilities
void ScreenToPt(int x, int y, double& px, double& py) {
  px = double(x - window_w / 2) / cam_zoom - cam_x;
  py = double(y - window_h / 2) / cam_zoom - cam_y;
}
void PtToScreen(double px, double py, int& x, int& y) {
  x = int(cam_zoom * (px + cam_x)) + window_w / 2;
  y = int(cam_zoom * (py + cam_y)) + window_h / 2;
}

//All fractal equations
void mandelbrot(double& x, double& y, double cx, double cy) {
  double nx = x*x - y*y + cx;
  double ny = 2.0*x*y + cy;
  x = nx;
  y = ny;
}
void burning_ship(double& x, double& y, double cx, double cy) {
  double nx = x*x - y*y + cx;
  double ny = 2.0*std::abs(x*y) + cy;
  x = nx;
  y = ny;
}
void feather(double& x, double& y, double cx, double cy) {
  std::complex<double> z(x, y);
  std::complex<double> z2(x*x, y*y);
  std::complex<double> c(cx, cy);
  std::complex<double> one(1.0, 0.0);
  z = z*z*z/(one + z2) + c;
  x = z.real();
  y = z.imag();
}
void sfx(double& x, double& y, double cx, double cy) {
  std::complex<double> z(x, y);
  std::complex<double> c2(cx*cx, cy*cy);
  z = z * (x*x + y*y) - (z * c2);
  x = z.real();
  y = z.imag();
}
void henon(double& x, double& y, double cx, double cy) {
  double nx = 1.0 - cx*x*x + y;
  double ny = cy*x;
  x = nx;
  y = ny;
}
void duffing(double& x, double& y, double cx, double cy) {
  double nx = y;
  double ny = -cy*x + cx*y - y*y*y;
  x = nx;
  y = ny;
}
void ikeda(double& x, double& y, double cx, double cy) {
  double t = 0.4 - 6.0 / (1.0 + x*x + y*y);
  double st = std::sin(t);
  double ct = std::cos(t);
  double nx = 1.0 + cx*(x*ct - y*st);
  double ny = cy*(x*st + y*ct);
  x = nx;
  y = ny;
}
void chirikov(double& x, double& y, double cx, double cy) {
  y += cy*std::sin(x);
  x += cx*y;
}
void doublependulum(double& x, double& y, double cx, double cy) {

}

//List of fractal equations
static const Fractal all_fractals[] = {
  mandelbrot,
  burning_ship,
  feather,
  sfx,
  henon,
  duffing,
  ikeda,
  chirikov,
  doublependulum,
};

//Change the fractal
void SetFractal(sf::Shader& shader, int type) {
  shader.setUniform("iType", type);
  jx = jy = 1e8;
  fractal = all_fractals[type];
  normalized = (type == 0);
  hide_orbit = true;
  frame = 0;
}

//Used whenever the window is created or resized
void resize_window(sf::RenderWindow& window, sf::RenderTexture& rt, const sf::ContextSettings& settings, int w, int h) {
  window_w = w;
  window_h = h;
  rt.create(w, h);
  window.setView(sf::View(sf::FloatRect(0, 0, (float)w, (float)h)));
  frame = 0;
}
void make_window(sf::RenderWindow& window, sf::RenderTexture& rt, const sf::ContextSettings& settings, bool is_fullscreen) {
  window.close();
  sf::VideoMode screenSize;
  if (is_fullscreen) {
    screenSize = sf::VideoMode::getDesktopMode();
    window.create(screenSize, window_name, sf::Style::Fullscreen, settings);
  } else {
    screenSize = sf::VideoMode(window_w_init, window_h_init, 24);
    window.create(screenSize, window_name, sf::Style::Resize | sf::Style::Close, settings);
  }
  resize_window(window, rt, settings, screenSize.width, screenSize.height);
  window.setFramerateLimit(target_fps);
  //window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
  window.requestFocus();
}

//Main entry-point
#if _WIN32
INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
#else
int main(int argc, char *argv[]) {
#endif
  //Make sure shader is supported
  if (!sf::Shader::isAvailable()) {
    std::cerr << "Graphics card does not support shaders" << std::endl;
    return 1;
  }

  //Load the vertex shader
  sf::Shader shader;
  if (!shader.loadFromFile("src/vert.glsl", sf::Shader::Vertex)) {
    std::cerr << "Failed to compile vertex shader" << std::endl;
    system("pause");
    return 1;
  }

  //Load the fragment shader
  if (!shader.loadFromFile("src/frag.glsl", sf::Shader::Fragment)) {
    std::cerr << "Failed to compile fragment shader" << std::endl;
    system("pause");
    return 1;
  }

  //Load the font
  sf::Font font;
  if (!font.loadFromFile("src/RobotoMono-Medium.ttf")) {
    std::cerr << "Failed to load font" << std::endl;
    system("pause");
    return 1;
  }

  //Create the full-screen rectangle to draw the shader
  sf::RectangleShape rect;
  rect.setPosition(0, 0);

  //GL settings
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 4;
  settings.majorVersion = 3;
  settings.minorVersion = 0;

  //Create the window
  sf::RenderWindow window;
  sf::RenderTexture renderTexture;
  bool is_fullscreen = false;
  bool toggle_fullscreen = false;
  make_window(window, renderTexture, settings, is_fullscreen);

  //Setup the shader
  shader.setUniform("iCam", sf::Vector2f((float)cam_x, (float)cam_y));
  shader.setUniform("iZoom", (float)cam_zoom);
  SetFractal(shader, starting_fractal);


  //Main Loop
  double px, py, orbit_x, orbit_y;
  bool leftPressed = false;
  bool dragging = false;
  bool juliaDrag = false;
  bool takeScreenshot = false;
  bool showHelpMenu = false;
  bool animating = false;

  sf::Vector2i prevDrag;
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::Resized) {
        resize_window(window, renderTexture, settings, event.size.width, event.size.height);
      } else if (event.type == sf::Event::KeyPressed) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (keycode == sf::Keyboard::Escape) {
          window.close();
          break;
        } else if (keycode == sf::Keyboard::Space && !animating) {
          animating = true;
          animation_frame = 1;
        } else if (keycode >= sf::Keyboard::Num1 && keycode <= sf::Keyboard::Num9) {
          SetFractal(shader, keycode - sf::Keyboard::Num1);
        } else if (keycode == sf::Keyboard::F11) {
          toggle_fullscreen = true;
        } else if (keycode == sf::Keyboard::D) {
          sustain = !sustain;
        } else if (keycode == sf::Keyboard::C) {
          use_color = !use_color;
          frame = 0;
        } else if (keycode == sf::Keyboard::R) {
          cam_x = cam_x_dest = 0.0;
          cam_y = cam_y_dest = 0.0;
          cam_zoom = cam_zoom_dest = 100.0;
          frame = 0;
        } else if (keycode == sf::Keyboard::J) {
          if (jx < 1e8) {
            jx = jy = 1e8;
          } else if (!animating) {
            juliaDrag = true;
            const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            ScreenToPt(mousePos.x, mousePos.y, jx, jy);
          }
          hide_orbit = true;
          frame = 0;
        } else if (keycode == sf::Keyboard::S) {
          takeScreenshot = true;
        } else if (keycode == sf::Keyboard::H) {
          showHelpMenu = !showHelpMenu;
        }
      } else if (event.type == sf::Event::KeyReleased) {
        if (event.key.code == sf::Keyboard::J && !animating) {
          juliaDrag = false;
          animation_frame = 1;
        }
      } else if (event.type == sf::Event::MouseWheelMoved) {
        cam_zoom_dest *= std::pow(1.1f, event.mouseWheel.delta);
        const sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
        cam_x_fp = mouse_pos.x;
        cam_y_fp = mouse_pos.y;
      } else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          leftPressed = true;
          hide_orbit = false;
          ScreenToPt(event.mouseButton.x, event.mouseButton.y, px, py);
          orbit_x = px;
          orbit_y = py;
        } else if (event.mouseButton.button == sf::Mouse::Middle) {
          prevDrag = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
          dragging = true;
        } else if (event.mouseButton.button == sf::Mouse::Right) {
          hide_orbit = true;
        }
      } else if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left) {
          leftPressed = false;
        } else if (event.mouseButton.button == sf::Mouse::Middle) {
          dragging = false;
        }
      } else if (event.type == sf::Event::MouseMoved) {
        if (animating) {
          continue;
        }
        if (leftPressed) {
          ScreenToPt(event.mouseMove.x, event.mouseMove.y, px, py);
          orbit_x = px;
          orbit_y = py;
        }
        if (dragging) {
          sf::Vector2i curDrag = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
          cam_x_dest += (curDrag.x - prevDrag.x) / cam_zoom;
          cam_y_dest += (curDrag.y - prevDrag.y) / cam_zoom;
          prevDrag = curDrag;
          frame = 0;
        }
        if (juliaDrag) {
          ScreenToPt(event.mouseMove.x, event.mouseMove.y, jx, jy);
          frame = 1;
        }
      }
    }

    //Apply zoom
    double fpx, fpy, delta_cam_x, delta_cam_y;
    ScreenToPt(cam_x_fp, cam_y_fp, fpx, fpy);
    cam_zoom = cam_zoom*0.8 + cam_zoom_dest*0.2;
    ScreenToPt(cam_x_fp, cam_y_fp, delta_cam_x, delta_cam_y);
    cam_x_dest += delta_cam_x - fpx;
    cam_y_dest += delta_cam_y - fpy;
    cam_x += delta_cam_x - fpx;
    cam_y += delta_cam_y - fpy;
    cam_x = cam_x*0.8 + cam_x_dest*0.2;
    cam_y = cam_y*0.8 + cam_y_dest*0.2;

    //Create drawing flags for the shader
    const bool hasJulia = (jx < 1e8);
    const bool drawJset = juliaDrag || animating;
    const bool drawMset = !drawJset;
    const int flags = (drawMset ? 0x01 : 0) | (drawJset ? 0x02 : 0) | (use_color ? 0x04 : 0);

    if (animating) {
      frame = 1;

      // get mouse position
      sf::Vector2i mousePos = sf::Mouse::getPosition(window);
      double sx = mousePos.x + 0.01 * window_w * std::sin(animation_frame * 2 * M_PI / 600);
      double sy = mousePos.y + 0.01 * window_h * std::cos(animation_frame * 2 * M_PI / 600);

      jx = double(sx - window_w / 2) / cam_zoom - cam_x;
      jy = double(sy - window_h / 2) / cam_zoom - cam_y;
    }

    //Set the shader parameters
    const sf::Glsl::Vec2 window_res((float)window_w, (float)window_h);
    shader.setUniform("iResolution", window_res);
    shader.setUniform("iCam", sf::Vector2f((float)cam_x, (float)cam_y));
    shader.setUniform("iZoom", (float)cam_zoom);
    shader.setUniform("iFlags", flags);
    shader.setUniform("iJulia", sf::Vector2f((float)jx, (float)jy));
    shader.setUniform("iIters", max_iters);
    shader.setUniform("iTime", frame);

    //Draw the full-screen shader to the render texture
    sf::RenderStates states = sf::RenderStates::Default;
    states.blendMode = (frame > 0 ? BlendAlpha : BlendIgnoreAlpha);
    states.shader = &shader;
    rect.setSize(window_res);
    renderTexture.draw(rect, states);
    renderTexture.display();

    //Draw the render texture to the window
    sf::Sprite sprite(renderTexture.getTexture());
    window.clear();
    window.draw(sprite, sf::RenderStates(BlendIgnoreAlpha));

    //Save screen shot if needed
    if (takeScreenshot) {
      window.display();
      const time_t t = std::time(0);
      const tm* now = std::localtime(&t);
      char buffer[128];
      std::strftime(buffer, sizeof(buffer), "pic_%m-%d-%y_%H-%M-%S.png", now);
      const sf::Vector2u windowSize = window.getSize();
      sf::Texture texture;
      texture.create(windowSize.x, windowSize.y);
      texture.update(window);
      texture.copyToImage().saveToFile(buffer);
      takeScreenshot = false;
    }

    //Draw help menu
    if (showHelpMenu) {
      sf::RectangleShape dimRect(sf::Vector2f((float)window_w, (float)window_h));
      dimRect.setFillColor(sf::Color(0,0,0,128));
      window.draw(dimRect, sf::RenderStates(BlendAlpha));
      sf::Text helpMenu;
      helpMenu.setFont(font);
      helpMenu.setCharacterSize(24);
      helpMenu.setFillColor(sf::Color::White);
      helpMenu.setString(
        "  H - Toggle Help Menu                Left Mouse - Click or drag to hear orbits\n"
        "  D - Toggle Audio Dampening        Middle Mouse - Drag to pan view\n"
        "  C - Toggle Color                   Right Mouse - Stop orbit and sound\n"
        "F11 - Toggle Fullscreen             Scroll Wheel - Zoom in and out\n"
        "  S - Save Snapshot\n"
        "  R - Reset View\n"
        "  J - Hold down, move mouse, and\n"
        "      release to make Julia sets.\n"
        "      Press again to switch back.\n"
        "  1 - Mandelbrot Set\n"
        "  2 - Burning Ship\n"
        "  3 - Feather Fractal\n"
        "  4 - SFX Fractal\n"
        "  5 - H�non Map\n"
        "  6 - Duffing Map\n"
        "  7 - Ikeda Map\n"
        "  8 - Chirikov Map\n"
        "  9 - Double Pendulum Fractal\n"
      );
      helpMenu.setPosition(20.0f, 20.0f);
      window.draw(helpMenu);
    }

    //Flip the screen buffer
    window.display();

    if (animating) {
      animation_frame += 1;
    }
    if (animation_frame > 600) {
      animating = false;
      animation_frame = 1;
    }

    //Update shader time if frame blending is needed
    const double xSpeed = std::abs(cam_x - cam_x_dest) * cam_zoom_dest;
    const double ySpeed = std::abs(cam_x - cam_x_dest) * cam_zoom_dest;
    const double zoomSpeed = std::abs(cam_zoom / cam_zoom_dest - 1.0);
    if (xSpeed < 0.2 && ySpeed < 0.2 && zoomSpeed < 0.002) {
      frame += 1;
    } else {
      frame = 1;
    }

    //Toggle full-screen if needed
    if (toggle_fullscreen) {
      toggle_fullscreen = false;
      is_fullscreen = !is_fullscreen;
      make_window(window, renderTexture, settings, is_fullscreen);
    }
  }

  return 0;
}
