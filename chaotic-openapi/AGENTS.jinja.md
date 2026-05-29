# Jinja style guide

`*.{hpp,cpp}.jinja` templates produce C++ source files when rendered.
Mixed jinja/C++ template source is hard to perceive for a human.
We use several rules to ease this task:

- Jinja files should be as small as possible. Duplicated code/types must be moved to static `include/**.hpp` files.
- Use 4 spaces for generated C++ code:
  ```cpp
  namespace ns {
  class X {
  public:
      void f() {
          g();
      }
  };
  } // namespace ns
  ```
- Use identation with spaces to reflect statement/expression nesting level (1 level = 2 spaces), E.g.:
  ```jinja
  // In file.hpp.jinja
  class X {
    {% if flag %}
      {% for item in items %}
        {{ item.cpp_type }} {{ item.cpp_name }};
      {% endfor %}
    {% endif %}
  };
  ```
- Leave empty line after closing statements `end*` (e.g. endif, endfor) unless followed by closing statement, "}", ">", or ";". E.g.:
  ```jinja
  class T {
    {% if is_enabled %}
      {% if f_flag %}
        f();
      {% endif %}

      {% if g_flag %}
        g();
      {% endif %}

      foo();

      {% if h_flag %}
        h();
      {% endif %}
    {% endif %}
  };
  ```
