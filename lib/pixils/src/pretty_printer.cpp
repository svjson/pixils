
#include <pixils/geom.h>
#include <pixils/pretty_printer.h>

#include <roo/form.h>
#include <roo/type.h>

namespace Pixils
{
  unsigned int COLUMN_THRESHOLD = 100;

  Color NONE;

  std::vector<TextLine> TextLine::split_lines(unsigned int line_width)
  {
    std::vector<TextLine> result;

    unsigned int w = 0;
    std::vector<TextSegment> line_segs;
    for (auto& segment : segments)
    {
      TextSegment c_segment = segment;
      while (w + c_segment.text.size() >= line_width)
      {
        line_segs.push_back(
          TextSegment{c_segment.text.substr(0, line_width - w), c_segment.color});
        result.push_back({line_segs});
        line_segs.clear();
        c_segment.text = c_segment.text.substr(line_width - w);
        w = 0;
      }

      line_segs.push_back(c_segment);
      w += c_segment.text.size();
    }

    if (line_segs.size())
    {
      result.push_back({line_segs});
    }
    return result;
  }

  ObjectPrinter::PrinterContext& ObjectPrinter::PrinterContext::newline()
  {
    this->lines.push_back({this->segments});
    this->segments.clear();
    this->line_started = false;
    return *this;
  }

  ObjectPrinter::PrinterContext& ObjectPrinter::PrinterContext::indent()
  {
    this->indentation += "  ";
    if (this->threshold > 1) this->threshold -= 2;
    return *this;
  }

  ObjectPrinter::PrinterContext& ObjectPrinter::PrinterContext::unindent()
  {
    if (this->indentation.size() > 2)
    {
      this->indentation = this->indentation.substr(0, this->indentation.size() - 2);
    }
    else
    {
      this->indentation = "";
    }

    this->threshold += 2;

    return *this;
  }

  ObjectPrinter::PrinterContext& ObjectPrinter::PrinterContext::space()
  {
    out(" ", NONE);
    return *this;
  }

  void ObjectPrinter::PrinterContext::out(const std::string& text, Color& color)
  {
    if (!this->line_started)
    {
      this->segments.push_back(TextSegment{indentation, NONE});
      this->line_started = true;
    }
    this->segments.push_back(TextSegment{text, color});
  }

  std::vector<TextLine> ObjectPrinter::pretty_print(Roo::sptr_ast_node& form)
  {
    PrinterContext ctx = {COLUMN_THRESHOLD};
    pretty_print(*form, ctx);
    if (ctx.segments.size()) ctx.newline();
    return ctx.lines;
  }

  std::vector<TextLine> ObjectPrinter::pretty_print(Roo::sptr_val& value)
  {
    PrinterContext ctx = {COLUMN_THRESHOLD};
    pretty_print(*value, ctx);
    if (ctx.segments.size()) ctx.newline();
    return ctx.lines;
  }

  void ObjectPrinter::pretty_print(Roo::Value& value, PrinterContext& ctx)
  {
    if (value.type == Roo::Value::Type::MAP)
    {
      bool split = value.to_string().size() > ctx.threshold;

      ctx.out("{", rtvalue_colors.at(value.type));
      if (split) ctx.newline().indent();

      const auto& children = value.elements();
      for (unsigned int i = 0; i < children.size(); i += 2)
      {
        pretty_print(*children.at(i), ctx);
        if (i + 1 < children.size())
        {
          ctx.space();
          pretty_print(*children.at(i + 1), ctx);
        }

        if (split)
          ctx.newline();
        else if (i + 2 < children.size())
          ctx.space();
      }

      if (split) ctx.unindent();
      ctx.out("}", rtvalue_colors.at(value.type));
    }
    else if (value.type == Roo::Value::Type::LIST ||
             value.type == Roo::Value::Type::VECTOR)
    {
      bool split = value.to_string().size() > ctx.threshold;
      const std::string lpar = value.type == Roo::Value::Type::LIST ? "(" : "[";
      const std::string rpar = value.type == Roo::Value::Type::LIST ? ")" : "]";

      ctx.out(lpar, rtvalue_colors.at(value.type));
      if (split) ctx.newline().indent();

      const auto& children = value.elements();
      for (unsigned int i = 0; i < children.size(); i++)
      {
        pretty_print(*children.at(i), ctx);
        if (split)
          ctx.newline();
        else if (i + 1 < children.size())
          ctx.space();
      }
      if (split) ctx.newline().unindent();
      ctx.out(rpar, rtvalue_colors.at(value.type));
    }
    else if (value.type == Roo::Value::Type::OBJECT ||
             value.type == Roo::Value::Type::FUNCTION)
    {
      pretty_print(*value.obj(), ctx);
    }
    else
    {
      ctx.out(value.to_string(), rtvalue_colors.at(value.type));
    }
  }

  void ObjectPrinter::pretty_print(Roo::AST::ASTNode& form, PrinterContext& ctx)
  {
    if (form.get_type() == Roo::Form::MAP || form.get_type() == Roo::Form::HOST_OBJECT)
    {
      std::string strlen = form.to_string();
      bool split = strlen.size() > ctx.threshold;

      ctx.out("{", form_colors.at(form.get_type()));
      if (split) ctx.newline().indent();

      std::vector<Roo::AST::ASTNode*> keys;

      Roo::sptr_ast_node_v children = form.get_children();
      for (unsigned int i = 0; i < children.size(); i += 2)
      {
        pretty_print(*children.at(i), ctx);
        ctx.space();
        pretty_print(*children.at(i + 1), ctx);

        if (split)
          ctx.newline();
        else if (i != children.size() - 2)
          ctx.space();
      }

      if (split) ctx.unindent();
      ctx.out("}", form_colors.at(form.get_type()));
    }
    else if (form.get_type() == Roo::Form::LIST ||
             form.get_type() == Roo::Form::VECTOR)
    {
      std::string strlen = form.to_string();
      bool split = strlen.size() > ctx.threshold;

      Roo::AST::Seq& sexp = form.as<Roo::AST::Seq>();

      ctx.out(sexp.lpar(), form_colors.at(form.get_type()));
      if (split) ctx.newline().indent();

      Roo::sptr_ast_node_v children = form.get_children();
      for (unsigned int i = 0; i < children.size(); i++)
      {
        pretty_print(*form.get_children().at(i), ctx);
        if (split)
          ctx.newline();
        else if (i != form.get_children().size() - 1)
          ctx.space();
      }
      if (split) ctx.newline().unindent();
      ctx.out(sexp.rpar(), form_colors.at(form.get_type()));
    }
    else
    {
      ctx.out(form.to_string(), form_colors.at(form.get_type()));
    }
  }

} // namespace Pixils
