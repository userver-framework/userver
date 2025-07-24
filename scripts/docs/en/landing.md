\htmlonly

<link rel="stylesheet" href="landing.css" />
    <main class="main">
      <section class="section info">
        <div class="info__block container">
          <div class="info__header">
            <div class="info__title"></div>
            <a
              href="de/d6a/md_en_2index.html"
              class="info__logo"
              title="Сlick on me to go to the documentation 😉"
              id="landing_logo_id"
            ></a>
          </div>
          <p class="info__paragraph paragraph mt">
            <span class="userver__title userver__title_p">userver</span> is the
            modern open source asynchronous framework with a rich set of
            abstractions for fast and comfortable creation of C++ microservices,
            services and utilities.
          </p>
          <div class="info__buttons mt">
            <a
              class="button"
              title="Go to the documentation"
              href="de/d6a/md_en_2index.html"
            >
              documentation
            </a>
            <a
              class="button button_outline generic_tg_link"
              title="Go to the userver Telegram"
              href="https://t.me/userver_en"
            >
              > Community
            </a>
          </div>
        </div>
      </section>
      <section class="section how container">
        <h2>How It Works</h2>
        <p class="how__info paragraph">
          The problem of efficient I/O interactions is solved transparently for
          the developers
        </p>
        <div class="how__codeblocks mt">
          <div class="codeblock codeblock_userver">
            <div class="codeblock__header">
              <span class="userver__title">🐙 userver</span>
            </div>
            <pre
              class="codeblock__body"
            ><code>
std::size_t Ins(storages::postgres::Transaction& tr,
                std::string_view key) {
  auto res = tr.Execute("INSERT INTO keys VALUES ($1)", key);
  return res.RowsAffected();
}</code></pre>
          </div>
          <div
            class="codeblock codeblock_overflow codeblock_grey codeblock_cpp"
          >
            <div class="codeblock__header">Classic C++</div>
            <pre class="codeblock__body"><code>template &lt;class OnSuccess&gt;
void Ins(storages::postgres::Transaction& tr,
         std::string_view key, OnSuccess&& on_success) {
  tr.Execute("INSERT INTO keys VALUES ($1)", key,
    [on_success = std::forward&lt;OnSuccess&gt;(on_success)]
    (const auto& res, const auto& error) {
      if (error) {
        report_error(error);
        return;
      }
      on_success(res.RowsAffected());
    }
  );
}</code></pre>
          </div>
        </div>
      </section>
      <section class="section values container">
        <h2>Values of <span class="userver__title">userver</span></h2>
        <div class="values__cards mt">
          <div class="values__card">
            <span class="values__icon thumbnail thumbnail_debugging"></span>
            <p class="values__cardinfo">
              Technologies for debugging and memory profiling a running
              production service
            </p>
          </div>
          <div class="values__card">
            <span class="values__icon thumbnail thumbnail_modern"></span>
            <p class="values__cardinfo">
              Write your first toy C++ service, evolve it into a production
              ready service.
            </p>
          </div>
          <div class="values__card">
            <span class="values__icon thumbnail thumbnail_asynchronous"></span>
            <p class="values__cardinfo">
              Efficient asynchronous drivers for databases (MongoDB, PostgreSQL,
              MySQL/MariaDB, Valkey, Redis, ClickHouse, YDB, SQLite ...) and
              data transfer protocols (HTTP, WebSockets, gRPC, TCP, AMQP-0.9.1,
              Apache Kafka, ...), tasks construction and cancellation.
            </p>
          </div>
          <div class="values__card">
            <span class="values__icon thumbnail thumbnail_plane"></span>
            <p class="values__cardinfo">
              Functionality to change the service configuration on-the-fly.
              Adjust options of the deadline propagation, timeouts,
              congestion-control without a restart.
            </p>
          </div>
          <div class="values__card">
            <span class="values__icon thumbnail thumbnail_tools"></span>
            <p class="values__cardinfo">
              Rich set of high-level components for caches, tasks, distributed
              locking, logging, tracing, statistics, metrics, JSON/YAML/BSON.
            </p>
          </div>
          <div class="values__card">
            <span class="values__icon thumbnail thumbnail_abstractions"></span>
            <p class="values__cardinfo">
              Comprehensive set of asynchronous low-level synchronization
              primitives and OS abstractions.
            </p>
          </div>
        </div>
      </section>
      <section class="section sample container">
        <h2>Service Example</h2>
        <p class="how__info paragraph">
          Simple 🐙 userver service that handles HTTP requests to "/kv" URL and responds with a key from database
        </p>
          <pre id="intro_sample" style=""><code>
#include &lt;userver/easy.hpp&gt;
#include "schemas/key_value.hpp"

int main(int argc, char* argv[]) {
    using namespace userver;

    easy::HttpWith&lt;easy::PgDep&gt;(argc, argv)
        // Handles multiple HTTP requests to `/kv` URL concurrently
        .Get("/kv", [](formats::json::Value request_json, const easy::PgDep&amp; dep) {
            // JSON parser and serializer are generated from JSON schema by userver
            auto key = request_json.As&lt;schemas::KeyRequest&gt;().key;

            // Asynchronous execution of the SQL query in transaction. Current thread
            // handles other requests while the response from the DB is being received:
            auto res = dep.pg().Execute(
                storages::postgres::ClusterHostType::kSlave,
                // Query is converted into a prepared statement. Subsequent requests
                // send only parameters in a binary form and meta information is
                // discarded on the DB side, significantly saving network bandwidth.
                "SELECT value FROM key_value_table WHERE key=$1", key
            );

            schemas::KeyValue response{key, res[0][0].As&lt;std::string&gt;()};
            return formats::json::ValueBuilder{response}.ExtractValue();
        });
}</code></pre>
      </section>
      <section class="section companies container">
        <h2>
          Brands and companies using <span class="userver__title">userver</span>
        </h2>
        <div class="companies__logos mt">
          <span class="logo logo_uber" title="Uber Russia"></span>
          <span class="logo logo_delivery" title="Delivery club"></span>
          <span class="logo logo_matchmaker" title="Matchmaker"></span>
          <span class="logo logo_yago" title="Yandex Go"></span>
        </div>
      </section>
      <section class="section feedback">
        <div class="feedback__wrapper container">
          <div class="feedback__content">
            <h2>Leave Your Feedback</h2>
            <p class="feedback__info">
              Your opinion will help to improve our service
            </p>
            <form class="feedback__form">
              <fieldset class="feedback__stars">
                <input
                  class="feedback__star"
                  type="radio"
                  name="rating"
                  value="1"
                  aria-label="1 star"
                  required
                />
                <input
                  class="feedback__star"
                  type="radio"
                  name="rating"
                  value="2"
                  aria-label="2 star"
                  required
                />
                <input
                  class="feedback__star"
                  type="radio"
                  name="rating"
                  value="3"
                  aria-label="3 star"
                  required
                />
                <input
                  class="feedback__star"
                  type="radio"
                  name="rating"
                  value="4"
                  aria-label="4 star"
                  required
                />
                <input
                  class="feedback__star"
                  type="radio"
                  name="rating"
                  value="5"
                  aria-label="5 star"
                  required
                />
              </fieldset>
              <a
                href="https://forms.yandex.ru/u/667d482fe010db2f53e00edf/"
                target="_blank"
                class="button feedback__button"
                title="Fill out the feedback form">
                Go to the feedback form >
              </a>
            </form>
          </div>
          <img class="feedback__image" src="feedback.svg" alt="userver logo" />
        </div>
      </section>
    </main>
    <!-- Highlight codeblocks -->
    <script src="highlight.min.js"></script>
    <script>
      document.querySelectorAll(".codeblock__body").forEach((el) => {
        hljs.highlightElement(el);
      });
      document.querySelectorAll("#intro_sample").forEach((el) => {
        hljs.highlightElement(el);
      });
    </script>
    <!-- Hide some blocks on landing page -->
    <script type="text/javascript">
      document.querySelector(".header").style.display = "none";
    </script>
\endhtmlonly
