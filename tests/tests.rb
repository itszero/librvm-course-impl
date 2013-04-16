#!/usr/bin/env ruby
require 'test/unit'
require 'fileutils'
include FileUtils

class RVMTest < Test::Unit::TestCase
  def self.startup
    msg = `(cd ..; make clean; make) 2>&1`
    raise "Failed to compile librvm.a\n#{msg}" unless File.exists?("../librvm.a")
  end

  def setup
    return unless self.method_name =~ /_c$/
    rm_r "tmp" if File.directory? "tmp"
    mkdir "tmp"
  end

  def teardown
    rm_r "tmp" if File.directory? "tmp"
  end

  Dir["src/*.c"].each do |file|
    define_method "test_#{File.basename(file, ".c")}_c" do

    end
  end

  def test_hello
    true
  end
end
