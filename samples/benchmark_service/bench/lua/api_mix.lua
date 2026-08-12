-- Rough api-4/api-16 mix: 3× baseline + 3× json + 2× async-db
local counter = 0

request = function()
  counter = counter + 1
  local i = ((counter - 1) % 8) + 1
  if i <= 3 then
    return wrk.format("GET", "/baseline11?a=13&b=42")
  elseif i <= 6 then
    return wrk.format("GET", "/json/10?m=3")
  else
    return wrk.format("GET", "/async-db?min=10&max=50&limit=10")
  end
end
