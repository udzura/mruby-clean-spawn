# mruby-clean-spawn   [![Build Status](https://travis-ci.org/udzura/mruby-clean-spawn.svg?branch=master)](https://travis-ci.org/udzura/mruby-clean-spawn)
CleanSpawn class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'udzura/mruby-clean-spawn'
end
```
## example
```ruby
p CleanSpawn.hi
#=> "hi!!"
t = CleanSpawn.new "hello"
p t.hello
#=> "hello"
p t.bye
#=> "hello bye"
```

## License
under the MIT License:
- see LICENSE file
