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
              MySQL/MariaDB (experimental), Redis, ClickHouse, ...) and data
              transfer protocols (HTTP, WEbSockets, gRPC, TCP, AMQP-0.9.1
              (experimental), Apache Kafka (experimental), ...), tasks construction and cancellation.
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
    </main>
    <!-- Highlight codeblocks -->
    <script src="highlight.min.js"></script>
    <script>
      document.querySelectorAll(".codeblock__body").forEach((el) => {
        hljs.highlightElement(el);
      });
    </script>
    <!-- Hide some blocks on landing page -->
    <script type="text/javascript">
      document.querySelector(".header").style.display = "none";
    </script>

\endhtmlonly
