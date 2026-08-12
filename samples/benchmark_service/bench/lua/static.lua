-- Paths available in samples/benchmark_service/data/static
local paths = {
  "/static/reset.css",
  "/static/logo.svg",
  "/static/footer.html",
  "/static/manifest.json",
}
local counter = 0

request = function()
  counter = counter + 1
  local path = paths[((counter - 1) % #paths) + 1]
  return wrk.format("GET", path, {["Accept-Encoding"] = "gzip, br"})
end
