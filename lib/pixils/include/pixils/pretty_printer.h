#ifndef PIXILS_PRETTY_PRINTER_H
#define PIXILS_PRETTY_PRINTER_H

#include <pixils/geom.h>

#include <lisple/type.h>
#include <map>
#include <string>
#include <vector>

namespace Lisple
{
  class Object;
}

namespace Pixils
{
  struct TextSegment
  {
    std::string text;
    Color& color;
  };

  struct TextLine
  {
    std::vector<TextSegment> segments;

    std::vector<TextLine> split_lines(unsigned int line_width);
  };

  class ObjectPrinter
  {
    struct PrinterContext
    {
      unsigned int threshold;
      std::vector<TextSegment> segments = {};
      std::vector<TextLine> lines = {};

      bool line_started = false;
      std::string indentation = "";
      void out(const std::string&, Color& color);

      PrinterContext& newline();
      PrinterContext& indent();
      PrinterContext& unindent();
      PrinterContext& space();
    };

    std::map<Lisple::Form, Color> form_colors = {
      {Lisple::Form::ANY, Color{0xff, 0xff, 0xff}},
      {Lisple::Form::ARRAY, Color{0xcc, 0x78, 0xd1}},
      {Lisple::Form::BOOLEAN, Color{0xff, 0xff, 0xff}},
      {Lisple::Form::B_FALSE, Color{0xff, 0xff, 0xff}},
      {Lisple::Form::B_TRUE, Color{0xff, 0xff, 0xff}},
      {Lisple::Form::CHAR, Color{0xde, 0xf4, 0xdf}},
      {Lisple::Form::DISCARD, Color{0x77, 0x77, 0x77}},
      {Lisple::Form::FUNCTION, Color{0x89, 0xab, 0xf9}},
      {Lisple::Form::HOST_OBJECT, Color{0x32, 0x62, 0xd0}},
      {Lisple::Form::KEY, Color{0x94, 0xd9, 0xcb}},
      {Lisple::Form::LIST, Color{0xf9, 0xd7, 0x49}},
      {Lisple::Form::MACRO, Color{0xbf, 0x94, 0xe5}},
      {Lisple::Form::MAP, Color{0x96, 0xcd, 0xf6}},
      {Lisple::Form::NIL, Color{0x77, 0x77, 0x77}},
      {Lisple::Form::NUMBER, Color{0xe9, 0x91, 0x73}},
      {Lisple::Form::STRING, Color{0xe6, 0xc5, 0x94}},
      {Lisple::Form::SYMBOL, Color{0xb6, 0xe9, 0x73}},
      {Lisple::Form::WORD, Color{0xd8, 0xde, 0xea}}};

    void pretty_print(Lisple::Object& form, PrinterContext& ctx);

   public:
    std::vector<TextLine> pretty_print(Lisple::sptr_sobject& form);
  };
} // namespace Pixils

#endif
