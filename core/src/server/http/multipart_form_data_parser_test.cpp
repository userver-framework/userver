#include <userver/utest/utest.hpp>

#include <server/http/multipart_form_data_parser.hpp>

USERVER_NAMESPACE_BEGIN

TEST(MultipartFormDataParser, ContentType) {
  namespace sh = server::http;
  EXPECT_TRUE(sh::IsMultipartFormDataContentType("multipart/form-data"));
  EXPECT_TRUE(sh::IsMultipartFormDataContentType(
      "multipart/form-data; "
      "boundary=------------------------8099aaf9723cd601"));
  EXPECT_FALSE(sh::IsMultipartFormDataContentType("multipart/form-data-bad"));
  EXPECT_FALSE(sh::IsMultipartFormDataContentType(
      "multipart/form-data-bad; "
      "boundary=------------------------8099aaf9723cd601"));
}

void DoParseOkTest(const std::string& content_type, const std::string& body,
                   std::optional<bool> strict_cr_lf = {}) {
  namespace sh = server::http;
  sh::FormDataArgs form_data_args;
  if (strict_cr_lf)
    ASSERT_TRUE(ParseMultipartFormData(content_type, body, form_data_args,
                                       *strict_cr_lf));
  else
    ASSERT_TRUE(ParseMultipartFormData(content_type, body, form_data_args));
  EXPECT_EQ(form_data_args.size(), 2);

  sh::FormDataArg arg_text;
  arg_text.value = "default";
  arg_text.content_disposition = R"(form-data; name="text")";
  EXPECT_EQ(form_data_args["text"], std::vector{arg_text});
  EXPECT_EQ(form_data_args["text"].front().Charset(), "UTF-8");

  sh::FormDataArg arg_file_html;
  arg_file_html.value = "<!DOCTYPE html><title>Content of a.html.</title>\n";
  arg_file_html.content_disposition =
      R"(form-data; name="file1"; filename="a.html")";
  arg_file_html.filename = "a.html";
  arg_file_html.content_type = "text/html";

  sh::FormDataArg arg_file_txt;
  arg_file_txt.value = "Content of a.txt.\n";
  arg_file_txt.content_disposition =
      R"(form-data; name="file1"; filename="a.txt")";
  arg_file_txt.filename = "a.txt";
  arg_file_txt.content_type = "text/plain";

  auto file1_files = std::vector{arg_file_html, arg_file_txt};
  ASSERT_EQ(form_data_args["file1"].size(), file1_files.size());
  EXPECT_EQ(form_data_args["file1"][0], file1_files[0])
      << "parsed {" << form_data_args["file1"][0].ToDebugString()
      << "} instead of {" << file1_files[0].ToDebugString() << '}';
  EXPECT_EQ(form_data_args["file1"][1], file1_files[1])
      << "parsed {" << form_data_args["file1"][1].ToDebugString()
      << "} instead of {" << file1_files[1].ToDebugString() << '}';
  EXPECT_EQ(form_data_args["file1"], file1_files);
}

TEST(MultipartFormDataParser, ParseOk) {
  const std::string kContentType =
      "multipart/form-data; boundary=------------------------8099aaf9723cd601";
  const std::string kBody =
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"text\"\r\n"
      "\r\n"
      "default\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\r\n"
      "Content-Type: text/html\r\n"
      "\r\n"
      "<!DOCTYPE html><title>Content of a.html.</title>\n\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a.txt.\n\r\n"
      "--------------------------8099aaf9723cd601--\r\n";

  DoParseOkTest(kContentType, kBody);
  DoParseOkTest(kContentType, kBody, true);
}

TEST(MultipartFormDataParser, ParseOkBoundaryQuoted) {
  const std::string kContentType =
      "multipart/form-data; "
      "boundary=\"------------------------8099aaf9723cd601\"";
  const std::string kBody =
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"text\"\r\n"
      "\r\n"
      "default\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\r\n"
      "Content-Type: text/html\r\n"
      "\r\n"
      "<!DOCTYPE html><title>Content of a.html.</title>\n\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a.txt.\n\r\n"
      "--------------------------8099aaf9723cd601--\r\n";

  DoParseOkTest(kContentType, kBody);
  DoParseOkTest(kContentType, kBody, true);
}

TEST(MultipartFormDataParser, ParseNonStrictCrEol) {
  const std::string kContentType =
      "multipart/form-data; boundary=------------------------8099aaf9723cd601";
  const std::string kBody =
      "--------------------------8099aaf9723cd601\r"
      "Content-Disposition: form-data; name=\"text\"\r"
      "\r"
      "default\r"
      "--------------------------8099aaf9723cd601\r"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\r"
      "Content-Type: text/html\r"
      "\r"
      "<!DOCTYPE html><title>Content of a.html.</title>\n\r"
      "--------------------------8099aaf9723cd601\r"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r"
      "Content-Type: text/plain\r"
      "\r"
      "Content of a.txt.\n\r"
      "--------------------------8099aaf9723cd601--\r";

  DoParseOkTest(kContentType, kBody);
  DoParseOkTest(kContentType, kBody, false);
}

TEST(MultipartFormDataParser, ParseNonStrictLfEol) {
  const std::string kContentType =
      "multipart/form-data; boundary=------------------------8099aaf9723cd601";
  const std::string kBody =
      "--------------------------8099aaf9723cd601\n"
      "Content-Disposition: form-data; name=\"text\"\n"
      "\n"
      "default\n"
      "--------------------------8099aaf9723cd601\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\n"
      "Content-Type: text/html\n"
      "\n"
      "<!DOCTYPE html><title>Content of a.html.</title>\n\n"
      "--------------------------8099aaf9723cd601\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\n"
      "Content-Type: text/plain\n"
      "\n"
      "Content of a.txt.\n\n"
      "--------------------------8099aaf9723cd601--\n";

  DoParseOkTest(kContentType, kBody);
  DoParseOkTest(kContentType, kBody, false);
}

TEST(MultipartFormDataParser, ParseStrictCrLfError) {
  namespace sh = server::http;
  const std::string kContentType =
      "multipart/form-data; boundary=------------------------8099aaf9723cd601";
  const std::string kBody =
      "--------------------------8099aaf9723cd601\n"
      "Content-Disposition: form-data; name=\"text\"\n"
      "\n"
      "default\n"
      "--------------------------8099aaf9723cd601\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\n"
      "Content-Type: text/html\n\n<!DOCTYPE html><title>Content of "
      "a.html.</title>\n"
      "\n"
      "--------------------------8099aaf9723cd601\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\n"
      "Content-Type: text/plain\n"
      "\n"
      "Content of a.txt.\n"
      "\n"
      "--------------------------8099aaf9723cd601--\n";

  sh::FormDataArgs form_data_args;
  ASSERT_FALSE(
      ParseMultipartFormData(kContentType, kBody, form_data_args, true));
}

TEST(MultipartFormDataParser, ParseLeadingTrailingTrash) {
  const std::string kContentType =
      "multipart/form-data; boundary=------------------------8099aaf9723cd601";
  const std::string kBody =
      "some trash\r\n"
      "many lines of trash\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"text\"\r\n"
      "\r\n"
      "default\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\r\n"
      "Content-Type: text/html\r\n"
      "\r\n"
      "<!DOCTYPE html><title>Content of a.html.</title>\n\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a.txt.\n\r\n"
      "--------------------------8099aaf9723cd601--\r\n"
      "more more trash";

  DoParseOkTest(kContentType, kBody);
  DoParseOkTest(kContentType, kBody, true);
}

TEST(MultipartFormDataParser, ParseLeadingTrailingTrash2) {
  const std::string kContentType =
      "multipart/form-data; boundary=------------------------8099aaf9723cd601";
  const std::string kBody =
      "Content-Disposition: form-data; name=\"text\"\r\n"
      "\r\n"
      "some trash\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"text\"\r\n"
      "\r\n"
      "default\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.html\"\r\n"
      "Content-Type: text/html\r\n"
      "\r\n"
      "<!DOCTYPE html><title>Content of a.html.</title>\n\r\n"
      "--------------------------8099aaf9723cd601\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a.txt.\n\r\n"
      "--------------------------8099aaf9723cd601--\r\n"
      "Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "some trash instead of content of a.txt.\r\n";

  DoParseOkTest(kContentType, kBody);
  DoParseOkTest(kContentType, kBody, true);
}

TEST(MultipartFormDataParser, ParseEmptyForm) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; boundary=zzz";
  const std::string kBody = "--zzz--\r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_TRUE(form_data_args.empty());
}

void DoParseEmptyData(std::string_view body) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; Boundary=zzz";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, body, form_data_args));
  EXPECT_EQ(form_data_args.size(), 1);

  sh::FormDataArg arg;
  arg.value = "";
  arg.content_type = "text/plain";
  arg.content_disposition = "form-data; name=arg";
  EXPECT_EQ(form_data_args["arg"], std::vector{arg})
      << "parsed {" << form_data_args["arg"][0].ToDebugString()
      << "} instead of {" << arg.ToDebugString() << '}';
}

TEST(MultipartFormDataParser, ParseEmptyData) {
  const std::string kBody =
      "--zzz\r\n"
      "Content-Disposition: form-data; name=arg\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "\r\n"
      "--zzz--\r\n";
  DoParseEmptyData(kBody);
}

TEST(MultipartFormDataParser, ParseEmptyDataNoFinalCrLf) {
  const std::string kBody =
      "--zzz\r\n"
      "Content-Disposition: form-data; name=arg\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "\r\n"
      "--zzz--";
  DoParseEmptyData(kBody);
}

TEST(MultipartFormDataParser, NoFinalCrLf2) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; boundary=zzz";
  const std::string kNoFinalCrLf = "--zzz--";
  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(
      ParseMultipartFormData(kContentType, kNoFinalCrLf, form_data_args));
  EXPECT_TRUE(form_data_args.empty());
}

TEST(MultipartFormDataParser, ParseNonUsAsciiCharsInHeaders) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; Boundary=zzz";
  const std::string kBody =
      "--zzz\r\n"
      "Content-Disposition: form-data; name=arg; filename=\"имя файла.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "содержимое файла\r\n"
      "--zzz--\r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_EQ(form_data_args.size(), 1);

  sh::FormDataArg arg;
  arg.value = "содержимое файла";
  arg.content_type = "text/plain";
  arg.content_disposition = "form-data; name=arg; filename=\"имя файла.txt\"";
  arg.filename = "имя файла.txt";
  EXPECT_EQ(form_data_args["arg"], std::vector{arg})
      << "parsed {" << form_data_args["arg"][0].ToDebugString()
      << "} instead of {" << arg.ToDebugString() << '}';
}

TEST(MultipartFormDataParser, ParseFileWithoutEolnEnding) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; Boundary=zzz";
  const char kBodyChars[] =
      "--zzz\r\n"
      "Content-Disposition: form-data; name=arg; filename=\"x.txt\"\r\n"
      "\r\n"
      "lines of\nfile content\0\r\n"
      "\r\r\n"
      "\r-\r\n"
      "-\r-\n-without\r\n"
      "--eoln in the end\r\n"
      "--zzz--\r\n";
  const std::string kBody{std::begin(kBodyChars),
                          std::prev(std::end(kBodyChars))};

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_EQ(form_data_args.size(), 1);

  sh::FormDataArg arg;
  const char kExpectedValueChars[] =
      "lines of\nfile content\0\r\n\r\r\n\r-\r\n-\r-\n-without\r\n--eoln in "
      "the end";
  const std::string kExpectedValue{std::begin(kExpectedValueChars),
                                   std::prev(std::end(kExpectedValueChars))};
  arg.value = kExpectedValue;
  arg.content_disposition = "form-data; name=arg; filename=\"x.txt\"";
  arg.filename = "x.txt";
  EXPECT_EQ(form_data_args["arg"], std::vector{arg})
      << "parsed {" << form_data_args["arg"][0].ToDebugString()
      << "} instead of {" << arg.ToDebugString() << '}';
}

TEST(MultipartFormDataParser, ParseExtraSpaces) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; boundary= zzz ";
  const std::string kBody =
      "--zzz\t  \t \r\n"
      "Content-Disposition:  \t \tform-data \t ; name  = \t\t \targ1 \t \r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      " some text \r\n"
      "--zzz  \t \r\n"
      "Content-Disposition: form-data; name=\" arg2 \"\r\nContent-Type: "
      "text/plain\r\n"
      "\r\n"
      "some text2\r\n"
      "--zzz-- \r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_EQ(form_data_args.size(), 2);

  sh::FormDataArg arg1;
  arg1.value = " some text ";
  arg1.content_type = "text/plain";
  arg1.content_disposition = "form-data \t ; name  = \t\t \targ1";
  ASSERT_TRUE(form_data_args.count("arg1"));
  EXPECT_EQ(form_data_args["arg1"], std::vector{arg1})
      << "parsed {" << form_data_args["arg1"][0].ToDebugString()
      << "} instead of {" << arg1.ToDebugString() << '}';

  sh::FormDataArg arg2;
  arg2.value = "some text2";
  arg2.content_type = "text/plain";
  arg2.content_disposition = "form-data; name=\" arg2 \"";
  EXPECT_EQ(form_data_args[" arg2 "], std::vector{arg2})
      << "parsed {" << form_data_args[" arg2 "][0].ToDebugString()
      << "} instead of {" << arg2.ToDebugString() << '}';
}

TEST(MultipartFormDataParser, ParseCharset) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; boundary=xxxxxx";
  const std::string kBody =
      "--xxxxxx\r\n"
      "Content-Disposition: Form-Data; name=\"arg1\"\r\n"
      "Content-Type: text/plain; charset=iso-8859-1\r\n"
      "\r\n"
      "some text\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=arg2; filename=\"a2.txt\"\r\n"
      "Content-Type: text/html; charset=koi8-r\r\n"
      "\r\n"
      "some koi8-r text\n\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg3\"; filename=\"a3.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a3.txt.\n\r\n"
      "--xxxxxx--\r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_EQ(form_data_args.size(), 3);

  ASSERT_FALSE(form_data_args["arg1"].empty());
  EXPECT_EQ(form_data_args["arg1"].front().Charset(), "iso-8859-1");

  ASSERT_FALSE(form_data_args["arg2"].empty());
  EXPECT_EQ(form_data_args["arg2"].front().Charset(), "koi8-r");

  ASSERT_FALSE(form_data_args["arg3"].empty());
  EXPECT_EQ(form_data_args["arg3"].front().Charset(), "UTF-8");
}

TEST(MultipartFormDataParser, ParseCharsetArgDefault) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; boundary=xxxxxx";
  const std::string kBody =
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg1\"\r\n"
      "Content-Type: text/plain; charset=iso-8859-1\r\n"
      "\r\n"
      "some text\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg2\"; filename=\"a2.txt\"\r\n"
      "Content-Type: text/html; charset=koi8-r\r\n"
      "\r\n"
      "some koi8-r text\n\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg3\"; filename=\"a3.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a3.txt.\n\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"_charset_\"\r\n"
      "\r\n"
      "iso-8859-4\r\n"
      "--xxxxxx--\r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_EQ(form_data_args.size(), 3);

  ASSERT_FALSE(form_data_args["arg1"].empty());
  EXPECT_EQ(form_data_args["arg1"].front().Charset(), "iso-8859-1");

  ASSERT_FALSE(form_data_args["arg2"].empty());
  EXPECT_EQ(form_data_args["arg2"].front().Charset(), "koi8-r");

  ASSERT_FALSE(form_data_args["arg3"].empty());
  EXPECT_EQ(form_data_args["arg3"].front().Charset(), "iso-8859-4");
}

TEST(MultipartFormDataParser, ParseCharsetContentTypeDefault) {
  namespace sh = server::http;
  const std::string kContentType =
      "multipart/form-data; boundary=xxxxxx; charset=iso-8859-4";
  const std::string kBody =
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg1\"\r\n"
      "Content-Type: text/plain; charset=iso-8859-1\r\n"
      "\r\n"
      "some text\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg2\"; filename=\"a2.txt\"\r\n"
      "Content-Type: text/html; charset=koi8-r\r\n"
      "\r\n"
      "some koi8-r text\n\r\n"
      "--xxxxxx\r\n"
      "Content-Disposition: form-data; name=\"arg3\"; filename=\"a3.txt\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Content of a3.txt.\n\r\n"
      "--xxxxxx--\r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_TRUE(ParseMultipartFormData(kContentType, kBody, form_data_args));
  EXPECT_EQ(form_data_args.size(), 3);

  ASSERT_FALSE(form_data_args["arg1"].empty());
  EXPECT_EQ(form_data_args["arg1"].front().Charset(), "iso-8859-1");

  ASSERT_FALSE(form_data_args["arg2"].empty());
  EXPECT_EQ(form_data_args["arg2"].front().Charset(), "koi8-r");

  ASSERT_FALSE(form_data_args["arg3"].empty());
  EXPECT_EQ(form_data_args["arg3"].front().Charset(), "iso-8859-4");
}

TEST(MultipartFormDataParser, ParseErrors) {
  namespace sh = server::http;
  const std::string kContentType = "multipart/form-data; boundary=zzz";
  const std::string kNoContentDispositionFormData =
      "--zzz\r\n"
      "Content-Disposition: no-form-data; name=\"arg\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "some text\r\n"
      "--zzz--\r\n";
  const std::string kNoData =
      "--zzz\r\n"
      "Content-Disposition: form-data; name=\"arg\"\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "--zzz--\r\n";

  sh::FormDataArgs form_data_args;
  ASSERT_FALSE(ParseMultipartFormData(
      kContentType, kNoContentDispositionFormData, form_data_args));
  EXPECT_TRUE(form_data_args.empty());

  ASSERT_FALSE(ParseMultipartFormData(kContentType, kNoData, form_data_args));
  EXPECT_TRUE(form_data_args.empty());
}

USERVER_NAMESPACE_END
