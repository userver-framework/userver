local counts = {25, 40, 50}
local counter = 0

request = function()
  counter = counter + 1
  local n = counts[((counter - 1) % #counts) + 1]
  return wrk.format("GET", "/json/" .. n .. "?m=4", {
    ["Accept-Encoding"] = "gzip, br",
  })
end
