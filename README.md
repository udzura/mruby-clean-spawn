# mruby-clean-spawn [![Build Status](https://travis-ci.org/udzura/mruby-clean-spawn.svg?branch=master)](https://travis-ci.org/udzura/mruby-clean-spawn)

Single function that spawns a new process with closing file descriptors 

## install by mrbgems

- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|
  conf.gem github: 'udzura/mruby-clean-spawn'
end
```

## example

```ruby
clean_spawn "/bin/bash", "-c", "ls -l"
# => get result
# => true

clean_spawn "/bin/bash", "-c" , "nohup sleep 30 &"
# => This can be a clean daemon, closing fds which are >= 3
```

## License
under the MIT License:
- see LICENSE file
