-- Body path injected via wrk -s after rewriting BODY_PATH below, or env.
local body_path = os.getenv("BENCH_UPLOAD_BODY") or "/dev/null"
local f = io.open(body_path, "rb")
local body = f and f:read("*a") or ""
if f then f:close() end

request = function()
  return wrk.format("POST", "/upload", {
    ["Content-Type"] = "application/octet-stream",
  }, body)
end
