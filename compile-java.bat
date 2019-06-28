SET PATH="C:\Program Files\Java\jdk1.8.0_121\bin\";%PATH%
cd src\java
javac -version
javac *.java
jar cvfm ..\..\lamp.jar META-INF\MANIFEST.MF *.class
del *.class
cd ..\..
pause