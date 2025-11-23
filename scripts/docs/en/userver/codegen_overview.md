# Codegen overview

Developing a good service requires many routine code and types to be written.
Sometimes it is handy to define types (and code) in declarative way out-of-code in external contracts and generate types/code from that.
It allows one to separate external contracts and business logic, and to define these contracts in language agnostic way. 

You may use code generators for the following activity:
* @ref scripts/docs/en/userver/chaotic.md
  * for PostgreSQL JSON/JSONB types
  * for dynamic configs 
  * for static configs
  * for Kafka/RabbitMQ messages
* @ref scripts/docs/en/userver/chaotic_clients.md
* @ref scripts/docs/en/userver/sql_files.md 

## userver_embed_file and embedding files

For embedding arbitrary data there is a `userver_embed_file` CMake function:

@snippet samples/embedded_files/CMakeLists.txt  embedded

To use the embedded file from C++ just include the generated header and call @ref utils::FindResource() function to
get the embedded file contents:

@snippet samples/embedded_files/main.cpp  embedded usage


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/periodics.md | @ref scripts/docs/en/userver/chaotic.md ⇨
@htmlonly </div> @endhtmlonly
