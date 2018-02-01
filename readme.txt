checkout 下来，编译，运行步骤
)../debug/touch hello			
)../src/proto/sh build.sh		#必须用这两部生成 hello 文件，和proto cpp文件，makefile才起作用。（待改善）
) ../debug/sh build.sh			#cmake创建
) ../sh restart					#根据依赖关系，生成proto,编译，重启
