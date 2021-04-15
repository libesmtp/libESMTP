# API Critique

Recently I have been revisiting aspects of libESMTP and considering what may or
may not be needed to bring the library up to date, since it is more than a
decade since it has had any development. I felt it might be useful to lay out
my observations and rationale for any changes.  I will also consider creating
language bindings since implementing these often gives some clarity on what
might be missing from a C API.

I feel that libESMTP has remained free of any significant bitrot but there
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

The number of calls in the API was starting to grow. This had bothered me as it
meant that the API would be harder to learn and went against the philosophy
that sending a mail sould be a simple thing to do. Nowadays, this aspect
bothers me less than it used to do, libESMTP still has a fairly small API by
modern standards and each API has a specific purpose with few arguments.

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
function acting on its object.  Basic use cases require only a fraction of the
full API to implement, in that respect I feel the original design goal is
achieved.

As originally designed, the libESMTP API created objects and configured them as
required, however few accessors were provided as it was expected that
applications would only query status after the fact when a session completed.
This has proven to be inconvenient in certain circumstances, for example, it is
difficult to query a recipient object for its mailbox address.  The lack of
accessors also complicates the event callback interface.  When writing a
binding for languages whose objects support properties, the lack of accessors
means properties are difficult to implement. This is discussed below for
Python.

----

## Naming Conventions

### Namespace

Since C has no concept of namespaces for functions or variables, all the API
calls are prefixed with `smtp_`.  I thought about using `esmtp_` but that
seemed too long!

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
oriented language bindings particularly difficult and non-obvious.  Had I
designed the API nowadays, I would follow the more rigorous approach used in
frameworks like GObject.

## Internal Interfaces

Unfortunately, libESMTP did not hide its internal interfaces from public
access, it is possible to link against any of its symbols at global scope.  It
is important to prevent linking against internal interfaces, since there may be
unintended conflicts with symbols in third-party libraries or application code.
The use of undocumented internal APIs can be a major barrier to development,
especially if significant applications rely on them.

Using a consistent prefix for public symbols makes it easy, using wildcards, to
create a linker map file to remove public references to internal calls, which
do not use the namespace prefix.  Without a consistent prefix, each publicly
available symbol must be explicitly listed, this is error prone since it is
easy to forget or misspell a name and there is no easy way to automatically
check this.

----

## Application Data

The application data API is problematic.  The idea was to set an arbitrary
pointer to data maintained by an application and associated with one of the
libESMTP objects. I followed a design pattern which set a pointer and returned
the old pointer which could then be deallocated or passed back to the
application.

Unfortunately this approach requires an application to iterate all the libESMTP
structures fetching back the pointer and deallocating it prior to destroying
the libESMTP session.  This significantly complicates use of libESMTP, is error
prone and subject to memory leaks if not executed correctly.  A better design
would provide a *release* function to free or otherwise release the application
data.  libESMTP can guarantee to call this at the correct time, significantly
easing the application design.

A well designed application data interface is important for many applications
and is especially so for implementing language bindings, particluarly as
neither side can control the other's memory management strategy and since weak
bindings may also be necessary.

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

## Type Safety

Although C is not a strongly typed language the libESMTP API is reasonably
type-safe. There are, however, two parts of the public API and a few internal
interfaces that use variadic functions, the event callback and the message
header API. The latter is discussed further below. Unfortunately these
interfaces cannot be validated for type-correct arguments either at compile
time or runtime.

The internal interfaces may be verified by careful code review and static
analysis, or by the compiler for the printf-like interfaces.  Unfortunately
this must also be done for every project using libESMTP.  Since the variadic
functions in the header API have only a few variations, a possible solution is
simply to provide a variant for each function for each combination of types.

----

## Callback Design

Ideally callbacks should be function closures, however C does not provide
closures. This is resolved by providing an extra closure argument when the
callback is registered and supplying that argument back to the callback. This
is conventional in many C language APIs.

### Event Callback Type Safety

Unfortunately the event callback's function is somewhat overloaded.  Similar
events often have inconsistent arguments and semantics and the remaining events
are special cases.

This design flaw is compounded since the varargs macros must be used to process
the callback arguments which is error prone, and impossible to check either at
compile time or run time.  Errors introduced writing the event callback might
only be detected when the application crashes.

This is particularly evident when trying to implement language bindings --
translating the event callback into something appropriate to the host language
becomes significantly more complex than it perhaps need to be, even for a
closely related language like C++.

Fortunately callbacks other than the event callback do not suffer this issue.

### Message Callback

I feel that I made my worst design decisions when implementing the message
callback.

Memory management in the message callback is poor. The first argument to the
callback is a pointer which can reference any arbitrary state the applicaton
maintains while reading the message.  However when libESMTP is finished reading
the message if that pointer is not NULL it is passed to 'free()'. This obliges
the callback to use 'malloc()' but what if the application wants to reference
its own private structures?  Furthermore the application must use the malloc
that pairs with the free libESMTP is linked against; sometimes this can be
problematic and usually only discovered when the application faults.

Another problem is that the message callback design means the callback must
allocate any state it requires on first call and so must check for first call
on every call.  Although the overhead of doing this is small, it complicates
the code and makes it harder to read, doubly so since this behaviour was never
properly documented.  This is a pity because libESMTP's internal logic knows
both when it is about to use the callback for the first time and when it has
finished reading the message.

Worse still, one of the arguments in the message callback is set to NULL to
signal that the input stream should be rewound to the beginning of input; once
more this strange behaviour is undocumented.

All of these objections could be improved by expanding the message callback
into three *open*, *read* and *close* phases and eliminating the implicit free.
The original API can be layered over this interface for backward compatibility.

### File and String Reader Callbacks

libESMTP provides two standard callbacks for reading messages from a string or
a FILE pointer.  On reflection, I think the APIs to set these callbacks should
have been provided as real functions in the library rather than as macros.
Had they been functions, it would have been possible to remedy the objections
discussed above in a new message API design and existing applications using
an updated libESMTP would automatically use the new interface. The macros have
made this impossible.

----

### Iterators

It is a pity that libESMTP did not implement iterators in its API.
In the libESMTP API, messages and recipients (and ETRN) may be iterated using
the 'enumerate' APIs. These take a callback function to process each item.

For example, because C does not provide function closures, a callback function
has no access to variables within the scope of the calling function.  One way
to work around this limitation is to put variables from the outer scope in a
struct and pass a pointer to the struct as the callback argument. It works but
it is always an ugly hack.

On reflection, a callback based design for iterating structures is really only
a good approach in languages that provide function closures, particularly those
that allow anonymous or lambda functions.

Iterators are almost always more convenient and intuitive to use and easier to
use in language bindings.

----

## Language Bindings

I think it is useful to consider providing language bindings for libESMTP.
Doing so, it became obvious that certain aspects of libESMTP's API design makes
this more difficult than it needs to be.

### Callbacks

The deficiencies in the event callback noted previously make binding to a
language's event mechanism more verbose than necessary and type safety is lost
in the process; typically this would have to be implemented as a 'big switch'
on the event type which selects the correct varargs statements to convert the
arguments.

### Host Language Objects

Because there is typically a one-to-one mapping between the bound language and
libESMTP's objects, they must be associated with each other. The application
data APIs may be used to do this. The deficiencies in the application data API
described above make this more difficult as the bound language's objects may be
reference counted or must be otherwise released to a garbage collector.

### Names

Interfacing the libESMTP objects to the host language equivalents should be
simple enough however, as previously noted, the lack of a rigorous naming
convention means that the mapping from API name to the host language's method
or property names for each object may not always be clear.

## Python

Python provides a comprehensive C API to integrate C libraries or code into
applications either directly or via its module system.  I feel that if all the
issues described are addressed this would be much simpler to implement. In
particular iterators and accessors, if available, would enable a much more
Pythonic result.

Interfacing callbacks should not be problematic, even with libESMTP's current
API, however the glue code would face issues with correctly handling variadic
callbacks.

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
recipient = Recipient(message, 'douglas@h2g2.info')

# ...

session.start()
```

Unfortunately the constructors bear little relation to the C equivalents nor do
the method API names have a clear mapping. Providing factory methods might be a
good idea, for example

``` python
message = session.add_message()
```

which has the benefit of being closer to the C API, and therefore more
intuitive, and requires fewer imports.

If libESMTP provided more accessors for some of its internal variables the
reverse path mailbox for a message could be more naturally expressed as a
property.  Even without accessors, this would still be possible but would
require the glue code to shadow the appropriate value which is both wasteful
and error prone.

``` python
message.reverse_path = 'brian@example.org'
print(message.reverse_path)
```

The lack of iterators makes adding Python iterators or generator expressions
difficult or impossible.  For instance, it should be possible to check
recipient status something like this:

``` python
session.start()

for message in session.messages():
    print('From:', message.reverse_path)
    for recipient in message.recipients():
        print('To:', recipient.mailbox, recipient.status)
```

or even use a comprehension

``` python
status = {recipient.mailbox: recipient.status for recipient in message.recipients()}
```

Interfacing a Python *file like* object to a revised message callback
should be a good test that the design is correct.


With these changes the example could be rewritten to be more Pythonic.

``` python
from libESMTP import Session

session = Session()
session.server = 'example.org:587'

# ...

message = session.add_message()
message.reverse_path = 'brian@example.org'
with open('message.txt','r') as filp:
    message.fromfile(filp)

recipient = message.add_recipient('douglas@h2g2.info')

# ...

session.start()

for message in session.messages():
    print('From:', message.reverse_path)
    for recipient in message.recipients():
        print('To:', recipient.mailbox, recipient.status)
```

### C++

While the libESMTP API can be used directly in C++, a proper language binding
would offer some benefits, especially since C++11 and later are arguably quite
different to earlier versions of the language.

Following some of the design patterns in the STL would provide a much more
natural API for modern C++, especially as libESMTP's objects share some
characteristics with STL containers.

Two STL patterns that come to mind are iterators and callbacks.  libESMTP's
session and message objects share characteristics of *std::forward_list* an
should follow their APIs.  Allowing callbacks to use *std::function* values
would permit use of lambda functions or any compatible object which overrides
the '()' operator.

Finally a C++ API could be created so that modern memory management practice
is observed, such as construction in-place and the no naked new or delete rule.

### Lua

Lua is an interesting case since it is a purely procedural language, however
its data structure, the table, adds extraordinary flexibility in the choice of
programming methodology. Lua also natively supports iterators.

Using Lua's C API it would be almost trivial to add a one-to-one mapping of C
APIs to Lua APIs but this would be a suboptimal approach.  Lua tables can act
both as an object and a type, via the use of metatables.  Therefore Lua
conventions are also better served by an object oriented approach but once
again the difficulties outlined above become apparent.

### GObject

GObject is a C framework rather than a language however it is a useful case to
consider. When integrating libESMTP into a GTK+ program, having GObject APIs
would be of some benefit. For example, making object state available as
properties, mapping the libESMTP callbacks to GObject's signal mechanism or
taking advantage of GObject reference counting,  introspection mechanisms and
language bindings, such as Vala. libESMTP sessions could even be constructed
using GBuilder XML files.

### Other Languages

Resolving issues for C++, Python and Lua should simplify writing bindings for
other languages, such as PHP, Ruby or D, especially if the basic set of
bindings were provided as a standard part of the libESMTP release.

----

## Conclusion

I feel it is useful to be self-critical of design decisions in various
projects. On reflection I think I got libESMTP mostly right but there are
deficiencies that are usefully remedied. This critique therefore serves as a
loose plan for further development.

All the topics above can be resolved by extending the libESMTP API to create a
strict superset and without having to alter the semantics of the existing API.
Accessors for the libESMTP object state, where missing, are easily added and
have no impact on the existing API. The event callback can be refactored into a
set of type safe callbacks.  An improved application data API has already been
designed and implemented.

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

