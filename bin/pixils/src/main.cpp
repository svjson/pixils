
#include <pixils/client.h>
#include <pixils/context.h>
#include <pixils/init_sdl.h>
#include <pixils/script.h>

#include <SDL2/SDL.h>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: pixils <script.lisple|directory>" << std::endl;
    return 1;
  }

  std::filesystem::path script_path(argv[1]);

  if (std::filesystem::is_directory(script_path)) script_path = script_path / "main.lisple";

  if (!std::filesystem::exists(script_path))
  {
    std::cerr << "Not found: " << script_path << std::endl;
    return 1;
  }

  script_path = std::filesystem::canonical(script_path);
  const std::string script_dir = script_path.parent_path().string();
  const std::string script_file = script_path.filename().string();

  auto opt_ctx = Pixils::init_sdl("Pixils");
  if (!opt_ctx.has_value())
  {
    std::cerr << "Failed to initialize SDL." << std::endl;
    SDL_Quit();
    return 1;
  }

  try
  {
    Pixils::RenderContext ctx = std::move(*opt_ctx);

    Lisple::Runtime runtime = Pixils::init_lisple_runtime(
      ctx,
      "main",
      [&script_dir](Pixils::RuntimeConfiguration* cfg) {
        cfg->load_path = {script_dir};
        cfg->asset_base_path = script_dir;
      },
      {script_file});

    Pixils::Client client(runtime, ctx);
    client.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    SDL_Quit();
    return 1;
  }

  SDL_Quit();
  return 0;
}
