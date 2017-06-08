##
## CleanSpawn Test
##

assert("CleanSpawn#clean_spawn") do
  ret = clean_spawn "/bin/bash", "-c", "ls -l >/dev/null"
  assert_true ret
end

assert("CleanSpawn#clean_spawn when failed") do
  ret = clean_spawn "/bin/bash", "-c", "nonexist 2>/dev/null"
  assert_false ret
end
