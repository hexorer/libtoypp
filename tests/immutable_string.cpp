#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>

#include "toypp/immutable_string.hpp"

TEST_CASE("tpp::ImmutableString") {
  const char* short_c_str = "hello";
  const char* long_c_str = "a quick brown fox quickly jumped over the lazy dog.";

  SECTION("semantics") {
    tpp::ImmutableString str{};
    CHECK(str.size() == 0);
    CHECK(str.data());

    tpp::ImmutableString short_str{short_c_str};
    CHECK(short_str.size() == std::strlen(short_c_str));
    CHECK(short_str.data());

    const char* long_c_str = "a quick brown fox quickly jumped over the lazy dog.";
    tpp::ImmutableString long_str{long_c_str};
    CHECK(long_str.size() == std::strlen(long_c_str));
    CHECK(long_str.data());

    tpp::ImmutableString long_str_by_strview{std::string_view(long_c_str)};
    CHECK(long_str_by_strview.size() == std::strlen(long_c_str));
    CHECK(long_str_by_strview.data());

    str = long_str;
    CHECK(str.size() == long_str.size());
    CHECK(str.data() == long_str.data());

    tpp::ImmutableString copied_str{str};
    CHECK(str.size() == copied_str.size());
    CHECK(str.data() == copied_str.data());

    tpp::ImmutableString moved_str{std::move(str)};
    CHECK(copied_str.size() == moved_str.size());
    CHECK(copied_str.data() == moved_str.data());

    const char* copied_str_data = copied_str.data();
    moved_str = std::move(copied_str);
    CHECK(moved_str.data() == copied_str_data);
  }

  SECTION("index") {
    CHECK(tpp::ImmutableString(short_c_str)[0] == short_c_str[0]);
    CHECK(tpp::ImmutableString(short_c_str)[1] == short_c_str[1]);
    CHECK(tpp::ImmutableString(short_c_str)[std::strlen(short_c_str)-1] == short_c_str[std::strlen(short_c_str)-1]);

    CHECK(tpp::ImmutableString(long_c_str)[0] == long_c_str[0]);
    CHECK(tpp::ImmutableString(long_c_str)[1] == long_c_str[1]);
    CHECK(tpp::ImmutableString(long_c_str)[std::strlen(long_c_str)-1] == long_c_str[std::strlen(long_c_str)-1]);
  }

  SECTION("comparison") {
    SECTION("empty") {
      CHECK(tpp::ImmutableString() == "");
      CHECK(tpp::ImmutableString() == std::string_view());
      CHECK(tpp::ImmutableString() == std::string());
      CHECK(tpp::ImmutableString() == tpp::ImmutableString());

      CHECK(tpp::ImmutableString("") == "");
      CHECK(tpp::ImmutableString("") == std::string_view());
      CHECK(tpp::ImmutableString("") == std::string());
      CHECK(tpp::ImmutableString("") == tpp::ImmutableString());

      CHECK(tpp::ImmutableString(std::string_view()) == "");
      CHECK(tpp::ImmutableString(std::string_view()) == std::string_view());
      CHECK(tpp::ImmutableString(std::string_view()) == std::string());
      CHECK(tpp::ImmutableString(std::string_view()) == tpp::ImmutableString());

      CHECK(tpp::ImmutableString(std::string()) == "");
      CHECK(tpp::ImmutableString(std::string()) == std::string_view());
      CHECK(tpp::ImmutableString(std::string()) == std::string());
      CHECK(tpp::ImmutableString(std::string()) == tpp::ImmutableString());
    }

    SECTION("short") {
      CHECK(tpp::ImmutableString("abcd") == "abcd");
      CHECK(tpp::ImmutableString("abcd") == std::string_view("abcd"));
      CHECK(tpp::ImmutableString("abcd") == std::string("abcd"));
      CHECK(tpp::ImmutableString("abcd") == tpp::ImmutableString("abcd"));

      CHECK(tpp::ImmutableString(std::string_view("abcd")) == "abcd");
      CHECK(tpp::ImmutableString(std::string_view("abcd")) == std::string_view("abcd"));
      CHECK(tpp::ImmutableString(std::string_view("abcd")) == std::string("abcd"));
      CHECK(tpp::ImmutableString(std::string_view("abcd")) == tpp::ImmutableString("abcd"));

      CHECK(tpp::ImmutableString(std::string("abcd")) == "abcd");
      CHECK(tpp::ImmutableString(std::string("abcd")) == std::string_view("abcd"));
      CHECK(tpp::ImmutableString(std::string("abcd")) == std::string("abcd"));
      CHECK(tpp::ImmutableString(std::string("abcd")) == tpp::ImmutableString("abcd"));
    }

    SECTION("long") {
      CHECK(tpp::ImmutableString(long_c_str) == long_c_str);
      CHECK(tpp::ImmutableString(long_c_str) == std::string_view(long_c_str));
      CHECK(tpp::ImmutableString(long_c_str) == std::string(long_c_str));
      CHECK(tpp::ImmutableString(long_c_str) == tpp::ImmutableString(long_c_str));

      CHECK(tpp::ImmutableString(std::string_view(long_c_str)) == long_c_str);
      CHECK(tpp::ImmutableString(std::string_view(long_c_str)) == std::string_view(long_c_str));
      CHECK(tpp::ImmutableString(std::string_view(long_c_str)) == std::string(long_c_str));
      CHECK(tpp::ImmutableString(std::string_view(long_c_str)) == tpp::ImmutableString(long_c_str));

      CHECK(tpp::ImmutableString(std::string(long_c_str)) == long_c_str);
      CHECK(tpp::ImmutableString(std::string(long_c_str)) == std::string_view(long_c_str));
      CHECK(tpp::ImmutableString(std::string(long_c_str)) == std::string(long_c_str));
      CHECK(tpp::ImmutableString(std::string(long_c_str)) == tpp::ImmutableString(long_c_str));
    }
  }

  SECTION("substr") {
    CHECK(tpp::ImmutableString().substr(0) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(0) == short_c_str);
    CHECK(tpp::ImmutableString(long_c_str).substr(0) == long_c_str);

    CHECK(tpp::ImmutableString().substr(1) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(1) == std::string_view(short_c_str).substr(1));
    CHECK(tpp::ImmutableString(long_c_str).substr(1) == std::string_view(long_c_str).substr(1));

    CHECK(tpp::ImmutableString().substr(4) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(4) == std::string_view(short_c_str).substr(4));
    CHECK(tpp::ImmutableString(long_c_str).substr(4) == std::string_view(long_c_str).substr(4));

    CHECK(tpp::ImmutableString().substr(0, 0) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(0, 0) == std::string_view(short_c_str).substr(0, 0));
    CHECK(tpp::ImmutableString(long_c_str).substr(0, 0) == std::string_view(long_c_str).substr(0, 0));

    CHECK(tpp::ImmutableString().substr(1, 0) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(1, 0) == std::string_view(short_c_str).substr(1, 0));
    CHECK(tpp::ImmutableString(long_c_str).substr(1, 0) == std::string_view(long_c_str).substr(1, 0));

    CHECK(tpp::ImmutableString().substr(1, 1) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(1, 1) == std::string_view(short_c_str).substr(1, 1));
    CHECK(tpp::ImmutableString(long_c_str).substr(1, 1) == std::string_view(long_c_str).substr(1, 1));

    CHECK(tpp::ImmutableString().substr(4, 4) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(4, 4) == std::string_view(short_c_str).substr(4, 4));
    CHECK(tpp::ImmutableString(long_c_str).substr(4, 4) == std::string_view(long_c_str).substr(4, 4));

    CHECK(tpp::ImmutableString().substr(1, 12000) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(1, 12000) == std::string_view(short_c_str).substr(1, 12000));
    CHECK(tpp::ImmutableString(long_c_str).substr(1, 12000) == std::string_view(long_c_str).substr(1, 12000));

    CHECK(tpp::ImmutableString().substr(12000, 1) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(12000, 1) == "");
    CHECK(tpp::ImmutableString(long_c_str).substr(12000, 1) == "");

    CHECK(tpp::ImmutableString().substr(12000, 12000) == "");
    CHECK(tpp::ImmutableString(short_c_str).substr(12000, 12000) == "");
    CHECK(tpp::ImmutableString(long_c_str).substr(12000, 12000) == "");
  }

  SECTION("substr shallow-copy") {
    tpp::ImmutableString str{long_c_str};
    tpp::ImmutableString substr;

    substr = str.substr(1);
    CHECK(str.data() + 1 == substr.data());
    CHECK(str.size() - 1 == substr.size());

    substr = str.substr(4);
    CHECK(str.data() + 4 == substr.data());
    CHECK(str.size() - 4 == substr.size());
  }

  SECTION("append") {
    CHECK(tpp::ImmutableString() + "" == "");
    CHECK(tpp::ImmutableString() + std::string_view() == "");
    CHECK(tpp::ImmutableString() + tpp::ImmutableString() == "");

    CHECK(tpp::ImmutableString(short_c_str) + "" == std::string(short_c_str));
    CHECK(tpp::ImmutableString(short_c_str) + std::string_view() == std::string(short_c_str));
    CHECK(tpp::ImmutableString(short_c_str) + tpp::ImmutableString() == std::string(short_c_str));

    CHECK(tpp::ImmutableString(short_c_str) + short_c_str == std::string(short_c_str) + short_c_str);
    CHECK(tpp::ImmutableString(short_c_str) + std::string_view(short_c_str) == std::string(short_c_str) + short_c_str);
    CHECK(tpp::ImmutableString(short_c_str) + tpp::ImmutableString(short_c_str) == std::string(short_c_str) + short_c_str);

    CHECK(tpp::ImmutableString(long_c_str) + "" == std::string(long_c_str));
    CHECK(tpp::ImmutableString(long_c_str) + std::string_view() == std::string(long_c_str));
    CHECK(tpp::ImmutableString(long_c_str) + tpp::ImmutableString() == std::string(long_c_str));

    CHECK(tpp::ImmutableString(long_c_str) + long_c_str == std::string(long_c_str) + long_c_str);
    CHECK(tpp::ImmutableString(long_c_str) + std::string_view(long_c_str) == std::string(long_c_str) + long_c_str);
    CHECK(tpp::ImmutableString(long_c_str) + tpp::ImmutableString(long_c_str) == std::string(long_c_str) + long_c_str);
  }

  SECTION("prepend") {
    CHECK("" + tpp::ImmutableString() == "");
    CHECK(std::string_view() + tpp::ImmutableString() == "");
    CHECK(tpp::ImmutableString() + tpp::ImmutableString() == "");

    CHECK("" + tpp::ImmutableString(short_c_str) == std::string(short_c_str));
    CHECK(std::string_view() + tpp::ImmutableString(short_c_str) == std::string(short_c_str));
    CHECK(tpp::ImmutableString() + tpp::ImmutableString(short_c_str) == std::string(short_c_str));

    CHECK(short_c_str + tpp::ImmutableString(short_c_str) == std::string(short_c_str) + short_c_str);
    CHECK(std::string_view(short_c_str) + tpp::ImmutableString(short_c_str) == std::string(short_c_str) + short_c_str);
    CHECK(tpp::ImmutableString(short_c_str) + tpp::ImmutableString(short_c_str) == std::string(short_c_str) + short_c_str);

    CHECK("" + tpp::ImmutableString(long_c_str) == std::string(long_c_str));
    CHECK(std::string_view() + tpp::ImmutableString(long_c_str) == std::string(long_c_str));
    CHECK(tpp::ImmutableString() + tpp::ImmutableString(long_c_str) == std::string(long_c_str));

    CHECK(long_c_str + tpp::ImmutableString(long_c_str) == std::string(long_c_str) + long_c_str);
    CHECK(std::string_view(long_c_str) + tpp::ImmutableString(long_c_str) == std::string(long_c_str) + long_c_str);
    CHECK(tpp::ImmutableString(long_c_str) + tpp::ImmutableString(long_c_str) == std::string(long_c_str) + long_c_str);
  }
}
