package com.sugarbear.app;

import com.getsentry.raven.Raven;
import com.getsentry.raven.RavenFactory;
import com.getsentry.raven.dsn.Dsn;
import com.getsentry.raven.event.Event;
import com.getsentry.raven.event.EventBuilder;
import com.getsentry.raven.event.interfaces.ExceptionInterface;
import com.getsentry.raven.event.interfaces.MessageInterface;
import com.getsentry.raven.jvmti.LocalsCache;
import com.getsentry.raven.jvmti.Frame;
import org.apache.log4j.varia.NullAppender;

public class App {
    private static Raven raven;

    public static void main(String... args) {
        org.apache.log4j.BasicConfigurator.configure(new NullAppender());

        // Creation of the client with a specific DSN
        String dsn = "https://WRONG:SECRET@app.getsentry.com/90673";
        raven = RavenFactory.ravenInstance(dsn);

        // It is also possible to use the DSN detection system like this
        raven = RavenFactory.ravenInstance();

        // Advanced: To specify the ravenFactory used
        raven = RavenFactory.ravenInstance(new Dsn(dsn), "com.getsentry.raven.DefaultRavenFactory");

        logException();
    }

    static void logException() {
        try {
            unsafeMethod();
        } catch (Exception e) {
            // This adds an exception to the logs
            // EventBuilder eventBuilder = new EventBuilder()
            //                 .withMessage("Exception caught")
            //                 .withLevel(Event.Level.ERROR)
            //                 .withLogger(App.class.getName())
            //                 .withSentryInterface(new ExceptionInterface(e));
            // raven.runBuilderHelpers(eventBuilder); // Optional
            // raven.sendEvent(eventBuilder.build());
            Frame frame = LocalsCache.getResult()[0];
            System.out.println("Java Code:");
            System.out.println("==========");
            System.out.format("Frame locals: %s\n", frame.namedLocals);
            System.out.format("Value of asdf: %s\n", frame.getLocal(0));
        }
    }

    static void unsafeMethod() {
        int asdf = 69;
        throw new UnsupportedOperationException("You shouldn't call that");
    }
}
