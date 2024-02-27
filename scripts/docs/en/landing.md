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
              class="button button_outline"
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
            ><code>std::size_t Ins(storages::postgres::Transaction& tr,
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
void Ins(storages::postgres::Transaction& tr
        , std::string_view key, OnSuccess&& on_success) {
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
              (experimental), ...), tasks construction and cancellation.
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
      <section class="section users container">
        <h2>Our Happy Users</h2>
        <div class="users__navbuttons">
          <button class="users__navbutton">
            <img src="arrow-left.svg" />
          </button>
          <button class="users__navbutton users__navbutton_right">
            <img src="arrow-right.svg" />
          </button>
        </div>
        <div class="users__feedbacks">
          <div class="userFeedback">
            <img class="userFeedback__image" src="feedback1.png" />
            <div class="userFeedback__person">
              <h3 class="userFeedback__name">Ivan Ivanov</h3>
              <div class="userFeedback__position">Developer</div>
            </div>
            <p class="userFeedback__about">
              A short quote about how convenient and cool userver is a very long
              quote about how convenient and cool userver isa very long quote
              about how convenient and cool userver is
            </p>
            <p class="userFeedback__text">«userver such wow, many cool»</p>
          </div>
          <div class="userFeedback">
            <img class="userFeedback__image" src="feedback2.png" />
            <div class="userFeedback__person">
              <h3 class="userFeedback__name">Petr Petrov</h3>
              <div class="userFeedback__position">Developer</div>
            </div>
            <p class="userFeedback__about">
              A short quote about how convenient and cool userver is a very long
              quote about how convenient and cool userver isa very long quote
              about how convenient and cool userver is
            </p>
            <p class="userFeedback__text">
              «userver is super, love it, recommend it»
            </p>
          </div>
        </div>
        <div class="pagination">
          <button class="pagination__button pagination__button_active"></button>
          <button class="pagination__button"></button>
          <button class="pagination__button"></button>
          <button class="pagination__button"></button>
        </div>
      </section>
      <section class="section companies container">
        <h2>
          Brands and companies using <span class="userver__title">userver</span>
        </h2>
        <div class="companies__logos mt">
          <span class="logo logo_uber" title="Uber"></span>
          <span class="logo logo_delivery" title="Delivery club"></span>
        </div>
      </section>
      <section class="section feedback">
        <div class="feedback__wrapper container">
          <h2 class="feedback__title">Leave Your Feedback</h2>
          <p class="feedback__info">
            Your opinion will help to improve our service
          </p>
          <form class="feedback__form">
            <label for="name">Name</label>
            <div class="input">
              <input type="text" name="name" id="name" placeholder="John Doe" />
            </div>
            <label for="email">E-Mail</label>
            <div class="input">
              <input
                type="email"
                name="email"
                id="email"
                placeholder="mail@mail.com"
              />
            </div>
            <label for="message">Message</label>
            <div class="input">
              <textarea
                name="message"
                id="message"
                placeholder="Leave your feedback here"
              ></textarea>
            </div>
            <button class="button feedback__submit">Send</button>
          </form>
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
    </script>
    <!-- Hide some blocks on landing page -->
    <script type="text/javascript">
      document.getElementById("side-nav").style.display = "none";
      document.querySelector(".header").style.display = "none";
    </script>

\endhtmlonly
