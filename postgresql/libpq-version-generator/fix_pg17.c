// hi from postgresql

extern const char * pg_encoding_to_char_private(int);

const char * pg_encoding_to_char(int encode){
  return pg_encoding_to_char_private(encode);
}

extern int	pg_char_to_encoding_private(const char *name);

int pg_char_to_encoding(const char *name){
  return pg_char_to_encoding_private(name);
}
