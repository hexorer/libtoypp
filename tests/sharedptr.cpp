#include <cstdint>

#include <catch2/catch_all.hpp>

#include "toypp/sharedptr.hpp"

TEST_CASE("tpp::SharedPtr")
{
  tpp::SharedPtr ptr = tpp::make_shared<std::size_t>(42);

  CHECK(ptr.use_count() == 1);

  tpp::SharedPtr ptr_copy = ptr;

  CHECK(ptr.get() == ptr_copy.get());
  CHECK(ptr.use_count() == 2);
  CHECK(ptr.use_count() == ptr_copy.use_count());

  tpp::SharedPtr ptr_moved = std::move(ptr);

  CHECK(ptr_moved.get() == ptr_copy.get());
  CHECK(ptr_moved.use_count() == 2);
  CHECK(ptr_moved.use_count() == ptr_copy.use_count());

  {
    tpp::SharedPtr ptr_another_copy = ptr_copy;

    CHECK(ptr_another_copy.get() == ptr_copy.get());
    CHECK(ptr_another_copy.get() == ptr_moved.get());
    CHECK(ptr_another_copy.use_count() == 3);
    CHECK(ptr_another_copy.use_count() == ptr_copy.use_count());
    CHECK(ptr_another_copy.use_count() == ptr_moved.use_count());
  }

  CHECK(ptr_copy.use_count() == 2);
  CHECK(ptr_moved.use_count() == 2);

  ptr_copy.reset();

  CHECK(ptr_copy.get() == nullptr);
  CHECK(ptr_copy.get() != ptr_moved.get());
  CHECK(ptr_copy.use_count() == 0);
  CHECK(ptr_moved.use_count() == 1);

  tpp::SharedPtr<std::size_t> ptr_empty{};
  CHECK(ptr_empty.get() == nullptr);
  CHECK(ptr_empty.use_count() == 0);
  CHECK(tpp::SharedPtr(ptr_empty).use_count() == 0);

  SECTION("custom deleter") {
    std::size_t counter = 0;
    auto deleter = [](std::size_t* counter_ptr) { ++(*counter_ptr); };

    {
      auto ptr = tpp::SharedPtr(&counter, deleter);
      CHECK(*ptr == 0);
      CHECK(counter == 0);
    }

    CHECK(counter == 1);

    auto ptr = tpp::SharedPtr(&counter, deleter);
    CHECK(counter == 1);

    ptr.reset(&counter, deleter);
    CHECK(counter == 2);

    ptr.reset();
    CHECK(counter == 3);
  }

  SECTION("array") {
    auto ptr_raw = new std::size_t[10];
    for (std::size_t index = 0; index < 10; ++index) {
      ptr_raw[index] = index;
    }
    tpp::SharedPtr ptr(ptr_raw);
    CHECK(ptr.get() == ptr_raw);
    for (std::size_t index = 0; index < 10; ++index) {
      CHECK(ptr[index] == index);
    }
  }

  SECTION("cast") {
    struct Base {
      std::size_t value = 0;
      virtual ~Base() = default;
    };
    struct Derived : public Base {};

    Derived* ptr_raw = new Derived {};
    ptr_raw->value = 42;

    tpp::SharedPtr<Derived> ptr{ptr_raw};
    tpp::SharedPtr<Base> base_ptr{ptr};

    CHECK(ptr->value == 42);
    CHECK(ptr->value == base_ptr->value);

    base_ptr = ptr;
    CHECK(base_ptr->value == 42);

    base_ptr = tpp::SharedPtr(ptr);
    CHECK(base_ptr->value == 42);

    tpp::SharedPtr<Base> another_base_ptr{tpp::SharedPtr(ptr)};

    tpp::SharedPtr<Base> yet_another_base_ptr{new Derived{}};
  }
}

TEST_CASE("tpp::WeakPtr")
{
  tpp::SharedPtr ptr = tpp::make_shared<std::size_t>(42);
  tpp::WeakPtr weak = ptr;
  tpp::WeakPtr weak_copy = weak;

  CHECK(!weak.expired());
  CHECK(weak.use_count() == 1);

  CHECK(!weak_copy.expired());
  CHECK(weak_copy.use_count() == 1);

  CHECK(weak.lock() == weak_copy.lock());

  CHECK(weak.lock().use_count() == 2);

  tpp::WeakPtr<std::size_t> weak_moved{};
  weak_moved = std::move(weak_copy);

  CHECK(weak.use_count() == 1);
  CHECK(weak_moved.use_count() == 1);

  CHECK(!weak.expired());
  CHECK(!weak_moved.expired());

  CHECK(weak.lock());
  CHECK(weak_moved.lock());

  ptr.reset();

  CHECK(weak.use_count() == 0);
  CHECK(weak_moved.use_count() == 0);

  CHECK(weak.expired());
  CHECK(weak_moved.expired());

  CHECK(!weak.lock());
  CHECK(!weak_moved.lock());

  SECTION("empty") {
    tpp::WeakPtr<std::size_t> weak_empty{};

    CHECK(weak_empty.use_count() == 0);
    CHECK(tpp::WeakPtr(weak_empty).use_count() == 0);

    CHECK(weak_empty.expired());
    CHECK(tpp::WeakPtr(weak_empty).expired());

    CHECK(!weak_empty.lock());
    CHECK(!tpp::WeakPtr(weak_empty).lock());
  }
}
