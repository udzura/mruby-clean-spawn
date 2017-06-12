MRuby::Build.new do |conf|
  toolchain :gcc
  conf.gembox 'default'
  conf.gem mgem: 'mruby-process'
  if ENV['TEST_SIGNAL']
    conf.gem mgem: 'mruby-signal'
  end
  conf.gem '../mruby-clean-spawn'
  conf.enable_debug
  conf.enable_test
end
