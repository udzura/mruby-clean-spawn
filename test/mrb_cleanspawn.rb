##
## CleanSpawn Test
##

assert("CleanSpawn#hello") do
  t = CleanSpawn.new "hello"
  assert_equal("hello", t.hello)
end

assert("CleanSpawn#bye") do
  t = CleanSpawn.new "hello"
  assert_equal("hello bye", t.bye)
end

assert("CleanSpawn.hi") do
  assert_equal("hi!!", CleanSpawn.hi)
end
