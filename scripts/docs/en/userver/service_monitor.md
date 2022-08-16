# Service statistics

If your service has a server::handlers::ServerMonitor configured, then you may
get the service statistics and metrics.


## Commands
server::handlers::ServerMonitor has a REST API:
```
GET /service/monitor/
GET /service/monitor?prefix={prefix}
```
Note that the server::handlers::ServerMonitor handler lives at the separate
`components.server.listener-monitor` address, so you have to request them using the
`listener-monitor` credentials. See @ref md_en_userver_tutorial_production_service
for more info on configuration and ideas on how to change the
`/service/monitor` handle path.

## Examples:

### Get all the metrics

The amount of metrics depends on components count, threads count,
utils::statistics::MetricTag usage and configuration options.
```
bash
$ curl http://localhost:8085/service/monitor | jq
```
```json
{
  "$version": 2,
  "congestion-control.rps.is-custom-status-activated": 0,
  "dns-client": {
    "replies": {
      "file": 0,
      "cached": 0,
      "cached-stale": 0,
      "cached-failure": 0,
      "network": 0,
      "network-failure": 0,
      "$meta": {
        "solomon_children_labels": "dns_reply_source"
      }
    }
  },
  "cpu_time_sec": 1.87,
  "rss_kb": 151724,
  "open_files": 80,
  "major_pagefaults": 0,
  "io_read_bytes": 0,
  "io_write_bytes": 225280,
  "cache": {
    "dynamic-config-client-updater": {
      "$meta": {
        "solomon_label": "cache_name"
      },
      "full": {
        "update": {
          "attempts_count": 4,
          "no_changes_count": 0,
          "failures_count": 0
        },
        "documents": {
          "read_count": 41,
          "parse_failures": 0
        },
        "time": {
          "time-from-last-update-start-ms": 59498,
          "time-from-last-successful-start-ms": 59498,
          "last-update-duration-ms": 8
        }
      },
      "incremental": {
        "update": {
          "attempts_count": 47,
          "no_changes_count": 47,
          "failures_count": 0
        },
        "documents": {
          "read_count": 0,
          "parse_failures": 0
        },
        "time": {
          "time-from-last-update-start-ms": 1710,
          "time-from-last-successful-start-ms": 1710,
          "last-update-duration-ms": 1
        }
      },
      "any": {
        "update": {
          "attempts_count": 51,
          "no_changes_count": 47,
          "failures_count": 0
        },
        "documents": {
          "read_count": 41,
          "parse_failures": 0
        },
        "time": {
          "time-from-last-update-start-ms": 1710,
          "time-from-last-successful-start-ms": 1710,
          "last-update-duration-ms": 8
        }
      },
      "current-documents-count": 17
    }
  },
  "httpclient": {
    "worker-0": {
      "timings": {
        "1min": {
          "p0": 0,
          "p50": 0,
          "p90": 0,
          "p95": 0,
          "p98": 0,
          "p99": 0,
          "p99_6": 0,
          "p99_9": 0,
          "p100": 0,
          "$meta": {
            "solomon_children_labels": "percentile",
            "solomon_skip": true
          }
        }
      },
      "errors": {
        "ok": 0,
        "host-resolution-failed": 0,
        "socket-error": 0,
        "timeout": 0,
        "ssl-error": 0,
        "too-many-redirects": 0,
        "unknown-error": 0,
        "$meta": {
          "solomon_children_labels": "http_error"
        }
      },
      "reply-statuses": {
        "500": 0,
        "401": 0,
        "400": 0,
        "200": 0,
        "$meta": {
          "solomon_children_labels": "http_code"
        }
      },
      "retries": 0,
      "pending-requests": 0,
      "last-time-to-start-us": 0,
      "event-loop-load": {
        "1min": "0.000000"
      },
      "sockets": {
        "close": 0,
        "throttled": 0,
        "active": 0,
        "open": 0
      },
      "$meta": {
        "solomon_label": "http_worker_id"
      }
    },
    "worker-1": {
      "timings": {
        "1min": {
          "p0": 0,
          "p50": 0,
          "p90": 0,
          "p95": 0,
          "p98": 0,
          "p99": 0,
          "p99_6": 0,
          "p99_9": 0,
          "p100": 0,
          "$meta": {
            "solomon_children_labels": "percentile",
            "solomon_skip": true
          }
        }
      },
      "errors": {
        "ok": 0,
        "host-resolution-failed": 0,
        "socket-error": 0,
        "timeout": 0,
        "ssl-error": 0,
        "too-many-redirects": 0,
        "unknown-error": 0,
        "$meta": {
          "solomon_children_labels": "http_error"
        }
      },
      "reply-statuses": {
        "500": 0,
        "401": 0,
        "400": 0,
        "200": 0,
        "$meta": {
          "solomon_children_labels": "http_code"
        }
      },
      "retries": 0,
      "pending-requests": 0,
      "last-time-to-start-us": 0,
      "event-loop-load": {
        "1min": "0.000000"
      },
      "sockets": {
        "close": 0,
        "throttled": 0,
        "active": 0,
        "open": 0
      },
      "$meta": {
        "solomon_label": "http_worker_id"
      }
    },
    "pool-total": {
      "timings": {
        "1min": {
          "p0": 1,
          "p50": 1,
          "p90": 1,
          "p95": 1,
          "p98": 1,
          "p99": 1,
          "p99_6": 1,
          "p99_9": 1,
          "p100": 1,
          "$meta": {
            "solomon_children_labels": "percentile",
            "solomon_skip": true
          }
        }
      },
      "errors": {
        "ok": 51,
        "host-resolution-failed": 0,
        "socket-error": 0,
        "timeout": 0,
        "ssl-error": 0,
        "too-many-redirects": 0,
        "unknown-error": 0,
        "$meta": {
          "solomon_children_labels": "http_error"
        }
      },
      "reply-statuses": {
        "$meta": {
          "solomon_children_labels": "http_code"
        }
      },
      "retries": 0,
      "pending-requests": 0,
      "last-time-to-start-us": 131,
      "event-loop-load": {
        "1min": "0.000003"
      },
      "sockets": {
        "close": 0,
        "throttled": 0,
        "active": 1,
        "open": 1
      },
      "$meta": {
        "solomon_skip": true
      }
    },
    "destinations": {
      "http://localhost:8083//configs/values": {
        "timings": {
          "1min": {
            "p0": 1,
            "p50": 1,
            "p90": 1,
            "p95": 1,
            "p98": 1,
            "p99": 1,
            "p99_6": 1,
            "p99_9": 1,
            "p100": 1,
            "$meta": {
              "solomon_children_labels": "percentile",
              "solomon_skip": true
            }
          }
        },
        "errors": {
          "ok": 51,
          "host-resolution-failed": 0,
          "socket-error": 0,
          "timeout": 0,
          "ssl-error": 0,
          "too-many-redirects": 0,
          "unknown-error": 0,
          "$meta": {
            "solomon_children_labels": "http_error"
          }
        },
        "reply-statuses": {
          "500": 0,
          "401": 0,
          "400": 0,
          "200": 51,
          "$meta": {
            "solomon_children_labels": "http_code"
          }
        },
        "retries": 0,
        "pending-requests": 0,
        "sockets": {
          "open": 1
        }
      },
      "$meta": {
        "solomon_children_labels": "http_destination",
        "solomon_skip": true
      }
    }
  },
  "httpclient-stats": {
    "destinations": {
      "$meta": {
        "solomon_children_labels": "http_destination",
        "solomon_skip": true
      }
    }
  },
  "engine": {
    "task-processors": {
      "by-name": {
        "monitor-task-processor": {
          "tasks": {
            "created": 26,
            "alive": 5,
            "running": 5,
            "queued": 0,
            "finished": 21,
            "cancelled": 7
          },
          "errors": {
            "wait_queue_overload": 0,
            "$meta": {
              "solomon_children_labels": "task_processor_error"
            }
          },
          "context_switch": {
            "slow": 55,
            "fast": 0,
            "spurious_wakeups": 0,
            "overloaded": 0,
            "no_overloaded": 11
          },
          "worker-threads": 1
        },
        "main-task-processor": {
          "tasks": {
            "created": 103,
            "alive": 10,
            "running": 10,
            "queued": 0,
            "finished": 93,
            "cancelled": 2
          },
          "errors": {
            "wait_queue_overload": 7,
            "$meta": {
              "solomon_children_labels": "task_processor_error"
            }
          },
          "context_switch": {
            "slow": 5695,
            "fast": 0,
            "spurious_wakeups": 0,
            "overloaded": 1,
            "no_overloaded": 1069
          },
          "worker-threads": 6
        },
        "fs-task-processor": {
          "tasks": {
            "created": 39,
            "alive": 1,
            "running": 1,
            "queued": 0,
            "finished": 38,
            "cancelled": 0
          },
          "errors": {
            "wait_queue_overload": 0,
            "$meta": {
              "solomon_children_labels": "task_processor_error"
            }
          },
          "context_switch": {
            "slow": 59,
            "fast": 0,
            "spurious_wakeups": 0,
            "overloaded": 0,
            "no_overloaded": 8
          },
          "worker-threads": 2
        },
        "$meta": {
          "solomon_children_labels": "task_processor",
          "solomon_skip": true
        }
      }
    },
    "coro-pool": {
      "coroutines": {
        "active": 16,
        "total": 5000
      }
    },
    "uptime-seconds": 249,
    "load-ms": 116
  },
  "server": {
    "connections": {
      "active": 0,
      "opened": 0,
      "closed": 0
    },
    "requests": {
      "active": 0,
      "avg-lifetime-ms": 0,
      "processed": 0,
      "parsing": 0
    }
  },
  "congestion-control": {
    "rps": {
      "is-enabled": 0,
      "is-activated": 0,
      "time-from-last-overloaded-under-pressure-secs": 3850,
      "states": {
        "no-limit": 250,
        "not-overloaded-no-pressure": 0,
        "not-overloaded-under-pressure": 0,
        "overloaded-no-pressure": 0,
        "overloaded-under-pressure": 0
      },
      "current-state": 0
    }
  },
  "http": {
    "by-path": {
      "/service/monitor/_": {
        "by-handler": {
          "handler-server-monitor": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 4,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 1,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/service/jemalloc/prof/_command_": {
        "by-handler": {
          "handler-jemalloc": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/service/inspect-requests": {
        "by-handler": {
          "handler-inspect-requests": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/hello": {
        "by-handler": {
          "handler-hello-sample": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/tests/control": {
        "by-handler": {
          "tests-control": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/service/dnsclient/_command_": {
        "by-handler": {
          "handler-dns-client-control": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/ping": {
        "by-handler": {
          "handler-ping": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      },
      "/service/log-level/_level_": {
        "by-handler": {
          "handler-log-level": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      }
    },
    "by-fallback": {
      "implicit-http-options": {
        "by-handler": {
          "handler-implicit-http-options": {
            "handler": {
              "all-methods": {
                "total": {
                  "reply-codes": {
                    "1xx": 0,
                    "2xx": 0,
                    "3xx": 0,
                    "4xx": 0,
                    "5xx": 0,
                    "other": 0,
                    "500": 0,
                    "401": 0,
                    "400": 0,
                    "$meta": {
                      "solomon_children_labels": "http_code"
                    }
                  },
                  "in-flight": 0,
                  "too-many-requests-in-flight": 0,
                  "rate-limit-reached": 0,
                  "timings": {
                    "1min": {
                      "p0": 0,
                      "p50": 0,
                      "p90": 0,
                      "p95": 0,
                      "p98": 0,
                      "p99": 0,
                      "p99_6": 0,
                      "p99_9": 0,
                      "p100": 0,
                      "$meta": {
                        "solomon_children_labels": "percentile",
                        "solomon_skip": true
                      }
                    }
                  },
                  "$meta": {
                    "solomon_skip": true
                  }
                },
                "$meta": {
                  "solomon_skip": true
                }
              }
            }
          }
        }
      }
    }
  }
}
```

### Get metrics with prefix
Prefixes are matched against the JSON keys of the first level of nesting.
```
bash
$ curl http://localhost:8085/service/monitor?prefix=dns | jq
```
```json
{
  "$version": 2,
  "dns-client": {
    "replies": {
      "file": 0,
      "cached": 0,
      "cached-stale": 0,
      "cached-failure": 0,
      "network": 0,
      "network-failure": 0,
      "$meta": {
        "solomon_children_labels": "dns_reply_source"
      }
    }
  }
}
```

