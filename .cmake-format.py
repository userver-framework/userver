# -----------------------------
# Options affecting formatting.
# -----------------------------
with section("format"):

  # Disable formatting entirely, making cmake-format a no-op
  disable = False

  # How wide to allow formatted cmake files
  line_width = 120

  # How many spaces to tab for indent
  tab_size = 4

  # If true, lines are indented using tab characters (utf-8 0x09) instead of
  # <tab_size> space characters (utf-8 0x20). In cases where the layout would
  # require a fractional tab character, the behavior of the  fractional
  # indentation is governed by <fractional_tab_policy>
  use_tabchars = False

  # If <use_tabchars> is True, then the value of this variable indicates how
  # fractional indentions are handled during whitespace replacement. If set to
  # 'use-space', fractional indentation is left as spaces (utf-8 0x20). If set
  # to `round-up` fractional indentation is replaced with a single tab character
  # (utf-8 0x09) effectively shifting the column to the next tabstop
  fractional_tab_policy = 'use-space'

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  dangle_parens = True
