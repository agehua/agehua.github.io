性能测试

Prerequisites

服务器环境

如果需要运行单页面的性能测试，项目 服务器的 DEBUG 模式必须为 开启

本地环境

Python 2.7+
pip
gprof2dot: https://pypi.python.org/pypi/gprof2dot/
graphviz: http://www.graphviz.org/
安装

Python 2.7 和 pip 在此略过

gprof2dot

pip install gprof2dot
graphviz

访问 http://www.graphviz.org/Download.php，同意其许可条款。
然后下载安装包，运行安装包一步步进行安装。这里不做笔墨。

使用

webconsole 服务器启动后，在任意url中，加入 ?_perf&_perf_save 这两个 query string，传入服务器，例如：

http://domain/project/?_perf&_perf_save

在性能测试(Profiler）完成后，将自动下载文件，其文件名按照如下方式组织：

Perf-[[别名]]-[[时间戳]].prof

其中，别名默认为 “Unnamed”，如果需要其它的别名（方便组织性能测试文件），可以指定 _perf_save 这个 query parameter，比如

http://domain/project/?_perf&_perf_save=Instances: 别名为 Instances
http://domain/project/volumes?_perf&_perf_save=Volumes: 别名为 Volumes
转换脚本

将以下脚本保存为 Perf2Png.cmd，使用说明在脚本内

@echo off
REM 使用方法 Perf2Png.cmd <.prof文件>

set file=%1
for /F %%i in ("%file%") do set name=%%~ni

echo Generating call graph: %name%.png...
gprof2dot -f pstats "%file%" | dot -Tpng -o "%name%".png
echo Done.
Demo

下图是访问 /project 生成的调用图：

