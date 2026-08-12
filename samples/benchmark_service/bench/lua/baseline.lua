-- HttpArena-like mixed GET / POST-CL / POST-chunked against /baseline11
local counter = 0
local bodies = {"20", "7", "99"}

request = function()
  counter = counter + 1
  local i = ((counter - 1) % 3) + 1
  if i == 1 then
    return wrk.format("GET", "/baseline11?a=13&b=42")
  elseif i == 2 then
    return wrk.format("POST", "/baseline11?a=13&b=42", {
      ["Content-Type"] = "text/plain",
      ["Content-Length"] = "2",
    }, bodies[1])
  else
    -- chunked: wrk always sends Content-Length; approximate with another POST
    return wrk.format("POST", "/baseline11?a=13&b=42", {
      ["Content-Type"] = "text/plain",
    }, bodies[2])
  end
end
