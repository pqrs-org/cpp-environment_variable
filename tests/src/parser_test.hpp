#pragma once

#include <boost/ut.hpp>
#include <pqrs/environment_variable.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;
using namespace pqrs::environment_variable::parser;
using namespace std::string_view_literals;

void run_parser_test(void) {
  expect(setenv("HOME", "/tmp/ut-home", 1) == 0_i);
  unsetenv("UNDEF");

  "scan_env_name/basic"_test = [] {
    auto s = std::string_view{"FOO1_BAR/BAZ"};
    expect(scan_env_name(s, 0).has_value()) << "should parse from pos 0";
    expect(*scan_env_name(s, 0) == 8);        // FOO1_BAR
    expect(!scan_env_name(s, 3).has_value()); // '1' is not a valid start
    expect(scan_env_name(s, 4).has_value());  // 'B' after '1'
    expect(*scan_env_name(s, 4) == 8);
    expect(!scan_env_name(s, s.size()));
  };

  "is_valid_env_name"_test = [] {
    expect(is_valid_env_name("A"sv));
    expect(is_valid_env_name("A1_B"sv));
    expect(!is_valid_env_name(""sv));
    expect(!is_valid_env_name("1ABC"sv));
    expect(!is_valid_env_name("A-B"sv)); // hyphen not allowed
  };

  "strip_quotes_no_decode"_test = [] {
    {
      auto [inner, kind] = strip_quotes_no_decode("\"hi\"");
      expect(inner == "hi"sv);
      expect(kind == quote_kind::dquote);
    }
    {
      auto [inner, kind] = strip_quotes_no_decode("'hi'");
      expect(inner == "hi"sv);
      expect(kind == quote_kind::single);
    }
    {
      auto [inner, kind] = strip_quotes_no_decode("hi");
      expect(inner == "hi"sv);
      expect(kind == quote_kind::none);
    }
  };

  "strip_eol_comment_inplace"_test = [] {
    std::string a = "KEY=val # comment";
    strip_eol_comment_inplace(a);
    expect(a == "KEY=val"sv);

    // escaped '#': odd number of backslashes → not a comment
    std::string b = R"(  KEY=val \# not comment  )";
    strip_eol_comment_inplace(b);
    expect(b == R"(KEY=val \# not comment)"sv);

    // even number of backslashes before '#': starts a comment
    std::string c = R"(KEY=val \\# comment)";
    strip_eol_comment_inplace(c);
    expect(c == R"(KEY=val \\)"sv);

    // inside quotes: not a comment
    std::string d = R"(KEY="x # y")";
    strip_eol_comment_inplace(d);
    expect(d == R"(KEY="x # y")"sv);

    // full-line comment
    std::string e = "   # only comment   ";
    strip_eol_comment_inplace(e);
    expect(e.empty());

    // trailing \r
    std::string f = "KEY=val # comment\r";
    strip_eol_comment_inplace(f);
    expect(a == "KEY=val"sv);
  };

  //
  // parse_value_with_expansion
  //

  "parse_value_with_expansion/none-quote"_test = [] {
    // $VAR, ${VAR}
    expect(parse_value_with_expansion("$HOME/bin", quote_kind::none) == "/tmp/ut-home/bin"sv);
    expect(parse_value_with_expansion("$HOMEAAAA", quote_kind::none) == ""sv);
    expect(parse_value_with_expansion("${HOME}AAAA", quote_kind::none) == "/tmp/ut-homeAAAA"sv);

    // undefined -> empty
    expect(parse_value_with_expansion("$UNDEF/bin", quote_kind::none) == "/bin"sv);
    expect(parse_value_with_expansion("${UNDEF}AAAA", quote_kind::none) == "AAAA"sv);

    // \$ suppression (odd backslashes)
    expect(parse_value_with_expansion(R"(\$HOME)", quote_kind::none) == "$HOME"sv);

    // even backslashes before $: one '\' remains, then expands
    expect(parse_value_with_expansion(R"(\\$HOME)", quote_kind::none) == R"(\/tmp/ut-home)"sv);

    // unknown escape: "\q" -> "q"
    expect(parse_value_with_expansion(R"(\q)", quote_kind::none) == "q"sv);

    // trailing single backslash
    expect(parse_value_with_expansion(R"(abc\)", quote_kind::none) == R"(abc\)"sv);

    // malformed ${ ...   (no closing '}') → treat '$' literally
    expect(parse_value_with_expansion(R"(${HOME)", quote_kind::none) == R"(${HOME)"sv);

    // lone '$' (next char invalid as name start)
    expect(parse_value_with_expansion(R"($-X)", quote_kind::none) == R"($-X)"sv);
  };

  "parse_value_with_expansion/double-quote"_test = [] {
    // escapes work inside double quotes
    expect(parse_value_with_expansion(R"(a\nb\tc)", quote_kind::dquote) == "a\nb\tc"sv);

    // escaped quote does not toggle the quote in comment stripper; here just decoded
    expect(parse_value_with_expansion(R"(quote: \")", quote_kind::dquote) == "quote: \""sv);

    // $ expands inside double quotes
    expect(parse_value_with_expansion(R"($HOME)", quote_kind::dquote) == "/tmp/ut-home"sv);
  };

  "parse_value_with_expansion/single-quote"_test = [] {
    // single quotes: no expansion, no escape decoding
    expect(parse_value_with_expansion(R"(a\n$HOME)", quote_kind::single) == R"(a\n$HOME)"sv);
  };

  //
  // parse_line
  //

  "parse_line/basic"_test = [] {
    // empty & comment lines
    expect(!parse_line("    "));
    expect(!parse_line("   # comment"));

    // no '='
    expect(!parse_line("export    "));
    expect(!parse_line("NOEQUALS"));

    // invalid key
    expect(!parse_line("1ABC=1"));

    // value comment stripping + trailing backslash via even '\\' before '#'
    {
      auto kv = parse_line(R"(C=val \\# comment)");
      expect(kv.has_value());
      expect(kv->first == "C"sv);
      expect(kv->second == R"(val \)"sv); // one backslash left after decoding
    }

    // ${HOME} expansion
    {
      auto kv = parse_line(R"(D=${HOME}/bin)");
      expect(kv.has_value());
      expect(kv->first == "D"sv);
      expect(kv->second == "/tmp/ut-home/bin"sv);
    }

    // undefined → empty
    {
      auto kv = parse_line(R"(E=$UNDEF:post)");
      expect(kv.has_value());
      expect(kv->first == "E"sv);
      expect(kv->second == ":post"sv);
    }

    // lone '$'
    {
      auto kv = parse_line(R"(F=$-BAD)");
      expect(kv.has_value());
      expect(kv->first == "F"sv);
      expect(kv->second == R"($-BAD)"sv);
    }

    // malformed ${... (name doesn't start with a valid char) → '$' literal keeps rest
    {
      auto kv = parse_line(R"(G=${1INVALID})");
      expect(kv.has_value());
      expect(kv->first == "G"sv);
      expect(kv->second == R"(${1INVALID})"sv);
    }

    // single quotes: no expansion/decoding; backslash+n and $ remain literal
    {
      auto kv = parse_line(R"(H='a\n$HOME')");
      expect(kv.has_value());
      expect(kv->first == "H"sv);
      expect(kv->second == R"(a\n$HOME)"sv);
    }

    // double quotes: escaped quote + '#' inside quotes; outer '# ...' stripped
    {
      auto kv = parse_line(R"(I="a \" # in"   # out)");
      expect(kv.has_value());
      expect(kv->first == "I"sv);
      expect(kv->second == "a \" # in"sv);
    }

    // \$ suppression parity (odd)
    {
      auto kv = parse_line(R"(K=\$HOME)");
      expect(kv.has_value());
      expect(kv->first == "K"sv);
      expect(kv->second == "$HOME"sv);
    }

    // \$ suppression parity (even) → one '\' left, then expands
    {
      auto kv = parse_line(R"(L=\\$HOME)");
      expect(kv.has_value());
      expect(kv->first == "L"sv);
      expect(kv->second == R"(\/tmp/ut-home)"sv);
    }
  };

  "parse_line/utf8"_test = [] {
    {
      auto kv = parse_line("U=📁 $HOME 📁");
      expect(kv.has_value());
      expect(kv->first == "U"sv);
      expect(kv->second == "📁 /tmp/ut-home 📁"sv);
    }
  };
}
} // namespace
