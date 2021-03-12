# Critique

## A Critique of the libESMTP API

Recently I have been revisiting aspects of libESMTP and considering what
may or may not be needed to bring the library up to date, since it is
more than a decade since it has had any development. I felt it might be
useful to lay out my observations and rationale for any changes.

I feel that libESMTP has remained free of any significant bit-rot but there
is definitely room for improvement.

----

## API Design

I wanted libESMTP to have a small API.  I reasoned that since sending an email
should be a simple thing to do, the API should be small.  Very early on I
realised that an SMTP interaction with an MSA (Message Submission Agent)
decomposed into one or more messages each with one or more recipients. This
mapped very nicely into three objects representing each of these concepts and
formed the basis of the API, each object being functionally distinct with their
own methods to configure them. Furthermore it was very natural that the SMTP
interaction, or *session* as I called it was a container for messages which in
turn were containers for their recipients.

Another benefit that became clear was that this approach allowed the entire
interaction to be controlled with a single API call.

At this point in the design the API consisted of functions to create and
configure each of the objects.  As an objective of libESMTP was to support a
comprehensive set of SMTP extensions, it transpired that quite a few APIs were
required to configure each of the libESMTP objects.

So far, so good. Originally I had intended that when a session was finished
the application could iterate the messages and recipients within the SMTP
session and discover whether messages had been transmitted successfully or
whether any or off the of the recipients were rejected of failed for any reason.
This proved to be satisfactory for simple command line type programs but not so
good  for interactive programs that might wish to present some sort of progress
indication or report status as the session progressed.  Therefore I introduced
a callback mechanism to report events as the protocol progressed.

A further callback was added so that the SMTP interaction could be monitored
while omitting the actual message transfer itself.

Another consideration was that although RFC 821 and RFC 822 and their
successors are orthogonal to each other, that is message format is independent
of the transmission protocol, there is slight crossover because an MTA is
required to add a Received: header to messages in transit, and certain headers
are required or prohibited in newly submitted messages.  While libESMTP can
handle most of this transparently to the application, it proved useful to add
API calls to provide finer control of this.

The number of calls in the
API was starting to grow. This had bothered me as it meant that the API would
be harder to learn and went against the philosophy that sending a mail sould be
a simple thing to do. Nowadays, this aspect bothers me less than it used to do,
libESMTP still has a fairly small API by modern standards and each API has a
specific purpose with few arguments.

### ABI

One of my earliest decisions was that the ABI should be unaffected by internal
changes in libESMTP, such as bug fixes or new features.  The easiest way to
achieve this was to make all libESMTP's structures opaque so that their content
was affected or accessed only through API calls.  The actual structure
definitions could be kept private to libESMTP's code. This was easily achieved
since C allows structure declarations without defining their content, as long
as you only manipulate pointers to the structures. Since they are still treated
as distinct types, errors in usage can be detected at compile time rather than
by faulting at runtime.

It still surprises me two decades later that I still occasionally find
libraries that fail to hide their structures as well as libESMTP, although
there are cases where this is not possible.

### Extensibility

The API has proven extensible since the associated objects can be modified, if
necessary, without breaking the ABI. New methods are easily added by following
the naming patterns for their respective API.

### Ease of Use

I feel that libESMTP's API is easy to use. Each object is logically distinct
and directly relates to SMTP concepts and each method has a single well defined
function acting on its object. However ease of use could be better had a more
consistent naming convention been used as described below.

As originally designed, the libESMTP API created objects and configured them as
required, however few accessors were provided as it was expected that
applications would only query status after the fact when a session completed.
This has proven to be inconvenient in certain circumstances, for example, it is
difficult to query a recipient object for its mailbox address.  The lack of
accessors also complicates the event callback interface.  When writing a
binding for languages whose objects support properties, the lack of accessors
means properties are difficult to implement. This is discussed below for
Python.

This deficiency should be addressed in a future release.

----

## Naming Conventions

### Namespace

Since C does not have any concept of namespaces for functions or variables, all
the API calls are prefixed with `smtp_`.  I thought about using `esmtp_` but
that seemed too long!  Although not implemented until quite recently, the
namespace prefix also allowed a linker map file to remove public references to
internal calls, which do not use the namespace prefix. A further advantage of
doing this is avoiding unintended conflicts with third-party libraries or
application code.  The use of undocumented internal APIs can be a major barrier
to development, especially if significant applications rely on them.

### Objects and Methods

As with namespaces, C has no concept of objects or classes. structs being the
most similar construct available. libESMTP's objects are implemented as structs
allocated and initialised by certain API calls, returning a pointer to the
application.  Pointers for the three struct types were declared as
`smtp_session_t`, `smtp_message_t` and `smtp_recipient_t` (an additional type
was created for the ETRN extension). 

On reflection, I feel I made a mistake in the API naming convention for the
object methods. I think I was trying to get them to read easily, since the
`namespace_object_method` convention tends to read backwards at times,
nevertheless this would probably have resulted in a more intuitive and more
easily memorised API.

These deficiencies make a simple mapping of API names to method names in object
oriented language bindings particularly difficult and non-obvious.  If I were
ever to rework the API I would follow the more rigorous approach used in
frameworks like GObject.

----

## User Data

The user data API is problematic.  The idea was to set an arbitrary pointer to
data maintained by an application and associated with one of the libESMTP
objects. I followed a design pattern which set a pointer and returned the old
pointer which could then be deallocated or passed back to the application.

Unfortunately this approach required an application to iterate all the libESMTP
structures fetching back the pointer and deallocating it prior to destroying
the libESMTP session.  This process is error prone and subject to memory leaks
if not executed correctly.

A well designed user data interface is important for many applications and is
especially so for implementing language bindings, particluarly as neither
side can control the other's memory management strategy and since weak bindings
may also be necessary.

Future releases will provide a better API to set user data, which allows an
application to provide a `destroy` function to release the referenced resource.
libESMTP can guarantee to call this at the correct time, significantly easing
the application design.

----

## Error Handling

Where API calls might fail they consistently return either zero or a NULL
pointer on failure, and non-zero/NULL values on success.  The actual reason for
failure is set in a global (or thread local) variable. This was modelled after
the standard C library's `errno` variable. I'm not sure whether this is a
satisfactory design but it does have precedent so it is a familiar pattern.

A second category of error occurs during a protocol exchange. This occurs when
the peer reports a problem via a status code, for example, a recipient mailbox
may not exist. libESMTP does not consider this an API failure, instead it
provides APIs to allow the application to query status after the protocol
session completes.  As noted above the event callback may be used to report
protocol status on-the-fly.

----

## Callback Design

Ideally callbacks should be function closures, however C does not provide
closures. This is resolved by providing an extra closure argument when the
callback is registered and supplying that argument back to the callback. This
is conventional in many C language APIs.

### Event Callback Type Safety

Unfortunately the event callback's function is somewhat overloaded, being
called with different arguments depending on the type of event.  This design
flaw is compounded by having to use the varargs interface to process the
callback arguments which is error prone, and cannot be checked either at
compile time or runtime.  This is the main area where the libESMTP API is not
type-safe.

Ideally the callback mechanism should be redesigned to avoid this. The most
obvious solution is tp provide a different callback for each event type while
retaining the original API for backward compatibility.

Callbacks other than the event callback do not suffer this issue.

### Message Callback

Memory management in the message callback is poor. The first argument to the
callback is a reference to a pointer which can be used to maintain state for
the callback. However when libESMTP is finished reading the message if the
pointer is not NULL, it is passed to 'free()'. This obliges the callback to use
'malloc()'. This is usually fine but the application must use the malloc that
pairs with the free libESMTP is linked against; sometimes this can be
problematic and usually only discovered when the application faults.

Another problem is that the message callback must allocate any state it
requires on first call so it must check for first call on every call.  Although
the overhead of doing this is small, it complicates the code and makes it
harder to read, doubly so since this behaviour was never properly documented.
This is a pity because libESMTP's internal logic knows both when it is about to
use the callback for the first time and when it has finished reading the
message.

All of these objections could be improved by expanding the message callback
into three *open*, *read* and *close* phases and eliminating the implicit free.
The original API can be layered over this interface for backward compatibility.

### File and String Reader Callbacks

libESMTP provides two standard callbacks for reading messages from a string or
a FILE pointer.  Looking back I think the APIs to set these callbacks should
have been provided as real functions in the library rather than as macros.

----

## Language Bindings

Recently I looked into providing language bindings for libESMTP.  It became
obvious that certain aspects of libESMTP's design makes this more difficult
than it needs to be.

### Iterators

In the C API, messages and recipients can be iterated using the 'enumerate'
APIs however these take a callback function to handle each item. This proves
inconvenient in some cases. For instance in C++ it might make sense to follow
the STL containers APIs to iterate the messages and recipients.  Without an
iterator API in libESMTP this is difficult to achieve.

In the specific case of C++, `std::function` could be used for the callback
allowing use of a lambda function which may be an acceptable alternative,
however this approach may not be possible or consistent with conventions in
other languages.

### Callbacks

The deficiencies in the event callback noted previously make binding to a
language's event mechanism more verbose than necessary and type safety is lost
in the process; typically this would have to be implemented as a 'big switch'
on the event type which selects the correct varargs statements to convert the
arguments.

If binding to C++, the obvious choice for the callback interface is to use
`std::function`.  Alternatively either the Boost or libsigc++ signal/slot
library could be used.

### OO Bindings

Interfacing the libESMTP objects to the host language equivalents is simple
enough however, as previously noted, the naming convention means that the
mapping from API name to method name for each object is not always clear.

For example the sequence

``` c
  smtp_session_t session;
  smtp_message_t message;
  smtp_recipient_t recipient;

  session = smtp_create_session ();
  smtp_set_server (session, "example.org:587");

  /* ... */

  message = smtp_add_message (session);
  smtp_set_reverse_path (message, "brian@example.org");
  recipient = smtp_add_recipient (message, "douglas@h2g2.info");

  /* ... */

  smtp_start_session (session);
```

might be rendered in Python as

``` python
from libESMTP import Session, Message, Recipient

session = Session()
session.set_server ('example.org:587')

# ...

message = Message(session)
message.set_reverse_path("brian@example.org")
recipient = Recipient('douglas@h2g2.info')

# ...

session.start()
```

Unfortunately the constructors bear little relation to the C equivalents nor do
the method API names have a clear mapping. For languages like Python providing
factory methods might be a good idea, for example

``` python
message = session.add_message()
```

which has the benefit of being closer to the C API and requires fewer imports.

As noted above the lack of accessors makes implementing properties difficult
for languages that support them.  For instance, in Python the reverse path
mailbox for a message is naturally expressed as a property:

``` python
message.reverse_path = 'brian@example.org'
print(message.reverse_path)
```

### Lua

Lua is an interesting case since it is a purely procedural language, however
its data structure, the table, adds extraordinary flexibility in the choice of
programming methodology.  Using Lua's C API it would be almost trivial to add a
one-to-one mapping of C APIs to Lua APIs but this would be a suboptimal
approach.  Lua tables can act both as an object and a type, via the use of
metatables.  Therefore Lua conventions are also better served by an OO
approach but once again the difficulties outlined above become apparent.

### GObject

GObject is a framework rather than a language however it is a useful case to
consider. When integrating libESMTP into a GTK+ program, having GObject APIs
would be of some benefit. For example, making object state available as
properties, mapping the libESMTP callbacks to GObject's signal mechanism or
taking advantage of GObject introspection mechanisms and language bindings,
such as Vala. libESMTP sessions could even be constructed using GBuilder XML
files.

### Other Languages

Considerations for C++, Python and Lua should be the same as for bindings to
other languages such as Ruby or D.

----

## Conclusion

I feel it is useful to be self-critical of design decisions in various
projects. On reflection I think I got libESMTP mostly right but there are
deficiencies that are usefully remedied. This critique therefore serves as a
loose plan for further development.

All the topics above can be resolved by extending the libESMTP API and without
having to alter the semantics of the existing API. Accessors for the libESMTP
object state, where missing, are easily added and have no impact on the
existing API. The event callback can be refactored into a set of type safe
callbacks.  An improved user data API has already been designed and
implemented.

Where features are updated, the original APIs can be retained with deprecated
status. This means the shared library can continue to work with existing
applications but use of deprecated features in new programs can be flagged with
compiler warnings or errors.

Although the API naming could be redesigned for better consistency and this
could be done retaining the original names with deprecated status, I don't
think it is worth the effort.  It would be confusing to have two names for many
APIs and documentation, always a weak spot in many projects, would be overly
complicated by this.

I feel that an important aspect of contemporary libraries is bindings for
multiple languages. It should be straightforward, having addressed the issues
outlined here, to provide official bindings for C++, Python, Lua and GObject.
These should also serve as effective models for further bindings.

None of these are large or difficult changes to implement but should
considerably enhance the API as libESMTP enters its third decade.

----

Brian Stafford, April 2021.

