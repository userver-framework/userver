local limits = {5, 10, 20, 35, 50}
local counter = 0

request = function()
  counter = counter + 1
  local lim = limits[((counter - 1) % #limits) + 1]
  return wrk.format("GET", "/async-db?min=10&max=50&limit=" .. lim)
end
