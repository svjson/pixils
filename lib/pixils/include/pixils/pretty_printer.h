#ifndef PIXILS_PRETTY_PRINTER_H
#define PIXILS_PRETTY_PRINTER_H

#include <pixils/geom.h>

#include <roo/type.h>
#include <map>
#include <string>
#include <vector>

namespace Roo::AST
{
  class ASTNode;
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

    std::map<Roo::Form, Color> form_colors = {
      {Roo::Form::ANY, Color{0xff, 0xff, 0xff}},
      {Roo::Form::VECTOR, Color{0xcc, 0x78, 0xd1}},
      {Roo::Form::BOOLEAN, Color{0xff, 0xff, 0xff}},
      {Roo::Form::B_FALSE, Color{0xff, 0xff, 0xff}},
      {Roo::Form::B_TRUE, Color{0xff, 0xff, 0xff}},
      {Roo::Form::CHAR, Color{0xde, 0xf4, 0xdf}},
      {Roo::Form::DISCARD, Color{0x77, 0x77, 0x77}},
      {Roo::Form::FUNCTION, Color{0x89, 0xab, 0xf9}},
      {Roo::Form::HOST_OBJECT, Color{0x32, 0x62, 0xd0}},
      {Roo::Form::KEYWORD, Color{0x94, 0xd9, 0xcb}},
      {Roo::Form::LIST, Color{0xf9, 0xd7, 0x49}},
      {Roo::Form::MACRO, Color{0xbf, 0x94, 0xe5}},
      {Roo::Form::MAP, Color{0x96, 0xcd, 0xf6}},
      {Roo::Form::NIL, Color{0x77, 0x77, 0x77}},
      {Roo::Form::NUMBER, Color{0xe9, 0x91, 0x73}},
      {Roo::Form::STRING, Color{0xe6, 0xc5, 0x94}},
      {Roo::Form::SYMBOL, Color{0xb6, 0xe9, 0x73}}};

    std::map<Roo::Value::Type, Color> rtvalue_colors = {
      {Roo::Value::Type::ANY, Color{0xff, 0xff, 0xff}},
      {Roo::Value::Type::BOOL, Color{0xff, 0xff, 0xff}},
      {Roo::Value::Type::CHAR, Color{0xde, 0xf4, 0xdf}},
      {Roo::Value::Type::FUNCTION, Color{0x89, 0xab, 0xf9}},
      {Roo::Value::Type::KEYWORD, Color{0x94, 0xd9, 0xcb}},
      {Roo::Value::Type::LIST, Color{0xf9, 0xd7, 0x49}},
      {Roo::Value::Type::MAP, Color{0x96, 0xcd, 0xf6}},
      {Roo::Value::Type::NATIVE_OBJECT, Color{0x32, 0x62, 0xd0}},
      {Roo::Value::Type::NIL, Color{0x77, 0x77, 0x77}},
      {Roo::Value::Type::NUMBER, Color{0xe9, 0x91, 0x73}},
      {Roo::Value::Type::OBJECT, Color{0x32, 0x62, 0xd0}},
      {Roo::Value::Type::STRING, Color{0xe6, 0xc5, 0x94}},
      {Roo::Value::Type::SYMBOL, Color{0xb6, 0xe9, 0x73}},
      {Roo::Value::Type::VECTOR, Color{0xcc, 0x78, 0xd1}}};

    void pretty_print(Roo::AST::ASTNode& form, PrinterContext& ctx);
    void pretty_print(Roo::Value& value, PrinterContext& ctx);

   public:
    std::vector<TextLine> pretty_print(Roo::sptr_ast_node& form);
    std::vector<TextLine> pretty_print(Roo::sptr_val& value);
  };
} // namespace Pixils

#endif
