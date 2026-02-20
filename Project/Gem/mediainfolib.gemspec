require 'fileutils'

Dir.mkdir('lib') unless File.exists?('lib')
FileUtils.cp('../../Source/MediaInfoDLL/MediaInfoDLL.rb', 'lib/mediainfolib.rb')

Gem::Specification.new do |s|
  s.name          = 'mediainfolib'
  s.version       = '18.08.1'
  s.license       = 'BSD-2-Clause'
  s.summary       = 'Get most relevant technical and tag data for video and audio files'
  s.author        = 'MediaArea.net SARL'
  s.email         = 'info@mediaarea.net'
  s.files         = ['lib/mediainfolib.rb']
  s.homepage      = 'https://mediaarea.net/MediaInfo'
  s.requirements  = 'MediaInfoLib native library in OS library search path'
  s.metadata      = {
    "homepage_uri"       => "https://mediaarea.net/MediaInfo",
    "changelog_uri"      => "https://mediaarea.net/MediaInfo/ChangeLog",
    "source_code_uri"    => "https://github.com/MediaArea/MediaInfoLib",
    "bug_tracker_uri"    => "https://github.com/MediaArea/MediaInfoLib/issues",
  }
  s.add_runtime_dependency 'ffi', '~> 1'
end
