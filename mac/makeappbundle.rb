#!/usr/bin/ruby
require 'ftools'
$VERBOSE = nil

binfilename = ARGV[0];

appdir = "./BioWatch.app";
contentsdir= "#{appdir}/Contents";
macosdir = "#{appdir}/Contents/MacOs";
frameworksdir = "#{appdir}/Contents/Frameworks";
pluginsdir = "#{appdir}/Contents/PlugIns";

#`rm -rf BioWatch.app`;
#`cp -r BioWatch.barebones.app BioWatch.app`
#Dir.mkdir(appdir)
#Dir.mkdir(contentsdir)
#Dir.mkdir(macosdir)
#Dir.mkdir(frameworksdir)
#Dir.mkdir(pluginsdir)

ignore=[/\/usr\/lib/, /\/System\/Library/, /executable_path/]

q = [binfilename]
#q.push("/opt/local/share/qt4/plugins/accessible/libqtaccessiblewidgets.dylib")
puts q;
while !(q.empty?)
  top = q.pop
  original_name=top
  puts "processing #{top}"

  if File.symlink?(top)
    top=File.readlink(top)
  end	
  basename = top.split("/")[-1]

#  basename_out = top.split("/")[-1]

  if !(File.exist?("#{macosdir}/#{basename}"))
    puts "copying #{top} to #{macosdir}"
    File.copy(original_name,"#{macosdir}/#{basename}");
    `chmod u+w #{macosdir}/#{basename}`;
  end
  otool_out = `otool -L #{macosdir}/#{basename}`;
  lines = otool_out.split("\n")[1..-1];
  files = lines.map{|l| l.split(" ")[0]}
  deps = files.select{|f| 
    (ignore.map{|i| (i=~f)==nil}).inject(true){|bmem,b| bmem && b} 
  }
  #puts "Found #{deps.size} deps: #{deps}"
  puts "Skipping #{files - deps}"
  `install_name_tool -id @executable_path/#{basename} #{macosdir}/#{basename}`
  #fix each dep reference
  deps.each{|d|
    deprealname=d;
    if File.symlink?(d)
       deprealname=File.readlink(d)
    end
    depbasename = deprealname.split("/")[-1]
    if depbasename != basename
      `install_name_tool -change #{d} @executable_path/#{depbasename} #{macosdir}/#{basename}`
      q.push(d)
      puts "  adding dep: #{d}"    
    end
  }

end




