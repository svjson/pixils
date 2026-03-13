
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{
  namespace Runtime
  {
    struct Mode;
  }
  struct RenderContext;

  void main_loop(Lisple::Runtime& runtime, Runtime::Mode& mode, RenderContext& ctx);
} // namespace Pixils

#endif
