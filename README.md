To compile the OSX native library:

```
mvn nar:nar-compile -P mac
```

To compile the java application:

```
mvn compile
```

To run the test application:

```
java -agentpath:target/nar/app-1.0-SNAPSHOT-x86_64-MacOSX-gpp-plugin/lib/x86_64-MacOSX-gpp/plugin/libapp-1.0-SNAPSHOT.bundle -jar target/app-1.0-SNAPSHOT.jar
```
