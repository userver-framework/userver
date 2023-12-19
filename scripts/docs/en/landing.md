<div id='mainDescription'>
<div id='landing-content'>
<div class="landing-description">The C++ Asynchronous Framework</div>

\htmlonly
<div class="landing-logo" id='landing_logo_id'>
  <a href="de/d6a/md_en_2index.html"><img src='logo.svg' alt='userver logo big'/></a>
</div>
\endhtmlonly

---

<div class="landing-text">
🐙 userver is the modern open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.
The problem of efficient I/O interactions is solved transparently for the
developers:</div>

<div class="landing-text">
<div class="fragment landing-fragment">
  <div class="line">
    std::size_t Ins(storages::postgres::Transaction& tr, std::string_view key) {
  </div>
  <div class="line">
    <span class="comment">  // Asynchronous execution of the SQL query in transaction. Current thread</span>
  </div>
  <div class="line">
    <span class="comment">  // handles other requests while the response from the DB is being received:</span>
  </div>
  <div class="line">
    <span class="keyword">  auto</span> res = tr.Execute(<span class="stringliteral">"INSERT INTO keys VALUES ($1)"</span>, key);
  </div>
  <div class="line">
    <span class="keyword">  return</span> res.RowsAffected();
  </div>
  <div class="line">
    }
  </div>
</div>
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
      Write @ref scripts/docs/en/userver/tutorial/hello_service.md "your first toy C++ service",
      evolve it into a @ref scripts/docs/en/userver/tutorial/production_service.md "production ready service".
  </div>
</div>

<div class="landing-container">
  <div class="landing-intro-left">
      Efficient asynchronous drivers for databases (MongoDB, PostgreSQL, MySQL/MariaDB (experimental), Redis, ClickHouse,
      ...) and data transfer protocols (HTTP, WEbSockets, gRPC, TCP, AMQP-0.9.1 (experimental), ...), tasks
      construction and cancellation.
  </div>
  <div class="landing-intro-right">
      Functionality to @ref scripts/docs/en/schemas/dynamic_configs.md "change the service configuration"
      on-the-fly. Adjust options of the deadline propagation, timeouts,
      congestion-control without a restart.
  </div>
</div>

<div class="landing-container">
  <div class="landing-intro-left">
      Rich set of high-level components for caches, tasks, distributed locking,
      logging, tracing, statistics, metrics, @ref scripts/docs/en/userver/formats.md "JSON/YAML/BSON".
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
      Dive into @ref scripts/docs/en/index.md "the documentation" for more details.
  
    \htmlonly
    <a href="https://github.com/userver-framework/" rel="noopener" target="_blank" class="titlelink">
      <img src="github_logo.svg"  width="48" height="48" class="gh-logo-landing" alt="Github"/>
    </a>
    &nbsp;
    <a href="https://t.me/userver_en" rel="noopener" id='telegram_channel' target="_blank" class="titlelink">
      <img src="telegram_logo.svg"  width="48" height="48" alt="Telegram"/>
    </a>
    \endhtmlonly
  </div>

</div>
</div>
</div>

\htmlonly
<script type="text/javascript">
  document.getElementById('side-nav').style.display = 'none';
  document.getElementById('mainDescription').closest('#doc-content').removeAttribute('id')
  document.getElementsByClassName('header')[0].style.display = 'none';
</script>
\endhtmlonly
