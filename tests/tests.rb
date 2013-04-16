#!/usr/bin/env ruby
require 'test/unit'
require 'fileutils'
include FileUtils

class RVMTest < Test::Unit::TestCase
  def self.startup
    msg = `(cd ..; make clean; make) 2>&1`
    raise "Failed to compile librvm.a\n#{msg}" unless $?.exitstatus == 0
  end

  def setup
    return unless self.method_name =~ /_c$/
    Dir.chdir "tests" unless Dir.pwd =~ /\/tests$/
    rm_r "tmp" if File.directory? "tmp"
    mkdir "tmp"

    src = self.method_name.split('_')[1..-1]
    ext = src.pop
    src = "#{File.join(src)}.#{ext}"

    Dir.chdir "tmp"
    compile src
  end

  def teardown
    Dir.chdir ".."
    rm_r "tmp" if File.directory? "tmp"
  end

  Dir["src/**/*.c"].each do |file|
    define_method "test_#{File.dirname(file).gsub(/\//, '_')}_#{File.basename(file, ".c")}_c" do
      msg = `./a.out 2>&1`
      assert_equal(0, $?.exitstatus, "Source: #{file}\nThis test terminated abnormally (#{$?.exitstatus}):\n#{msg}")
    end
  end

  def test_hello
    true
  end

  private
  def compile(src)
    system "gcc -o a.out ../#{src} ../../librvm.a -I../../ -I../#{File.dirname(src)}/../"
    raise "Compile Error" unless $?.exitstatus == 0
  end
end
