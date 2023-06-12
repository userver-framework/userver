\htmlonly
<script type="text/javascript">
  document.body.style.backgroundColor = 'black';
  document.getElementById('titlearea').style.display = 'none';
  document.getElementsByClassName('header')[0].style.display = 'none';
</script>
\endhtmlonly

<div class="landing-description">The C++ Asynchronous Framework</div>

\htmlonly
<div class="landing-logo" id='landing_logo_id'>
  <a href="d6/d2f/md_en_index.html"><img src='/logo.svg' alt='userver logo big'/></a>
</div>
\endhtmlonly

---

<div class="landing-text">
🐙 userver is the modern open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.
The problem of efficient I/O interactions is solved transparently for the
developers:</div>

<div class="landing-text">
<pre style="padding: 20px; color: silver;white-space: pre-wrap; word-wrap: break-word;">
std::size_t Ins(storages::postgres::Transaction& tr, std::string_view key) {
    <span style="color: darkgoldenrod">// Asynchronous execution of the SQL query in transaction. Current thread</span>
    <span style="color: darkgoldenrod">// handles other requests while the response from the DB is being received:</span>
    <span style="color: #008000">auto</span> res = tr.Execute(<span style="color: darkgreen">"INSERT INTO keys VALUES ($1)"</span>, key);
    <span style="color: #008000">return</span> res.RowsAffected();
}
</pre>
</div>

<!--@snippet postgresql/src/storages/postgres/tests/landing_test.cpp  Landing sample1 -->

<!--div class="landing-text"><div class="landing-motto">Fast. Reliable. Yours!</div></div-->

---

<div class="landing-container">
  <div class="landing-intro-center">
      Micro-services based on userver served more than a billion requests while
      you were reading this sentence.
  </div>
</div>

---

<div class="landing-container">
  <div class="landing-intro-left">
      Technologies for debugging and memory profiling a running production
      service.
  </div>
  <div class="landing-intro-right">
      Write @ref md_en_userver_tutorial_hello_service "your first toy C++ service",
      evolve it into a @ref md_en_userver_tutorial_production_service "production ready service".
  </div>
</div>

<div class="landing-container">
  <div class="landing-intro-left">
      Efficient asynchronous drivers for databases (MongoDB, PostgreSQL, MySQL/MariaDB (experimental), Redis, ClickHouse,
      ...) and data transfer protocols (HTTP, GRPC, TCP, AMQP-0.9.1 (experimental), ...), tasks
      construction and cancellation.
  </div>
  <div class="landing-intro-right">
      Functionality to @ref md_en_schemas_dynamic_configs "change the service configuration"
      on-the-fly. Adjust options of the deadline propagation, timeouts,
      congestion-control without a restart.
  </div>
</div>

<div class="landing-container">
  <div class="landing-intro-left">
      Rich set of high-level components for caches, tasks, distributed locking,
      logging, tracing, statistics, metrics, @ref md_en_userver_formats "JSON/YAML/BSON".
  </div>
  <div class="landing-intro-right">
      Comprehensive set of asynchronous low-level synchronization primitives
      and OS abstractions.
  </div>
</div>


---
<div class="landing-container">
  <div class="landing-intro-center">
      Speed of C++, simplicity of Python.
  </div>

  <div class="landing-intro-center">
      Dive into @ref md_en_index "the documentation" for more details.
  
    \htmlonly
    <a href="https://github.com/userver-framework/" rel="noopener" target="_blank" class="titlelink">
      <img src="/github_logo.svg"  width="48" height="48" style="filter: invert(100%);" alt="Github"/>
    </a>
    &nbsp;
    <a href="https://t.me/userver_en" rel="noopener" id='telegram_channel' target="_blank" class="titlelink">
      <img src="/telegram_logo.svg"  width="48" height="48" alt="Telegram"/>
    </a>
    \endhtmlonly
  </div>

</div>
