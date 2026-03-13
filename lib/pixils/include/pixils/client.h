
#ifndef __PIXILS__CLIENT_H_
#define __PIXILS__CLIENT_H_

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{
  struct RenderContext;

  void main_loop(Lisple::Runtime& runtime, RenderContext& ctx);
} // namespace Pixils

#endif
