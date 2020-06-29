# Retrospective

The following is an account of my experience with **libESMTP** as it
approaches about 20 years since I started work on it.

-----

During the late 1990s and early 2000s I was working on email based technologies
and had consequently taken an active interest in a number of
[IETF](https://www.ietf.org/) working groups, including those revising RFC 821
and RFC 822 which were at the time about 16 years old. I was also fortunate to
have had a supportive employer at that time who was able to introduce me to one
of the UK based IETF participants and who allowed me to attend a number of IETF
meetings internationally. My first IETF was, appropriately enough for a fan of
Douglas Adams' books, IETF 42 in Chicago.

IETF participation was, in retrospect, an accelerated learning and profoundly
useful experience. I was able to meet and discuss protocol development with
some of the key personalities from the early days of internet development, both
on a technical and philosophical level. By becoming involved in standards
development I also learned how to critically read and, more importantly,
understand RFCs and standards in general.

Having acquired a good working knowledge of, and the rationale for, various IETF
protocols I wanted to find a way to encapsulate that knowledge in a useful way.
Consequently I started work on libESMTP as I was unhappy with coding quality or
performance of the programs and libraries which were available at that time.

## Efficiency

A significant design goal was to be efficient on the network; most programs
available at that time performed very poorly if network latency was high, for
example on the wide-area network, or if the line turn-around time was high as
is typical of dial-up half-duplex modems, leading to long delays as the TCP
handshake took place.

Consequently libESMTP has always, *by design*, supported buffering and
pipelining SMTP commands which maximises network packet size, reduces the
number of TCP handshakes, and with pipelining packs multiple SMTP commands into
a single network packet.  This made dramatic improvements to submission times
on dial up connections, from minutes to seconds in some cases.

A 14k4 half-duplex modem takes about 100-300ms to turn round the line.  Similar
figures apply to the 56k modems also available back then.  A TCP handshake
takes two line turnarounds followed by one more to transmit the next packet.  I
had made a few measurements which showed that for a typical SMTP client there
were small bursts of data, usually no more than 20-70 bytes followed by long
idle periods - sometimes as much as one second.  Overall data transfer was poor
at a few hundred bytes per second, despite the modem's underlying bit rate. On
the same connection libESMTP was able to fully saturate the bandwidth with
data, performing line turnaround only as strictly necessary.

## Code Quality

I put considerable effort into compiling strictly ISO-C / Posix / SUS compliant
code, with maximum pedantic compiler errors enabled, and a zero tolerance
approach to any compiler warnings. It was painful adjusting to this at first
but it paid off and I would reccommend anyone to do the same and see it
through.

## Release

I made the first release of libESMTP in February 2001.

I had initially planned to use [Sourceforge](https://sourceforge.net/) to host
the project, however libESMTP used [OpenSSL](https://www.openssl.org/) and the
[Crypto Wars](https://en.wikipedia.org/wiki/Crypto_Wars) were well under
way at the time.  Unfortunately Sourceforge was hosted in the US and I wanted
to avoid issues surrounding US crypto law, so I looked round for another host
and found [UK Linux](http://www.uklinux.net/).  Better still, UK Linux had free
hosting for open-source projects.

## ![](pillarbox.png) Logo

All good projects need a logo. Most mail related programs use icons based on
the US style mailboxes. Since libESMTP is a bit different to mail related
software of that era, I decided a related but different logo was in order.
The red UK pillarbox seemed appropriate as it's somewhere you post mail rather
than receive it.  I took a photograph of one and made the PNG from that with
[the Gimp](https://www.gimp.org/).  The actual pillar box is somewhere in the
North of England.

I chose the name "libESMTP" because "libsmtp" was already taken. But since I
was supporting many protocol extensions, the 'E' felt appropriate.  To further
emphasise the unique character of the project I chose the unusual
capitalisation and this is how libESMTP should always be spelt.

> Normal grammatical and punctuation rules don't apply to brand names and
> logos. When I say that "libESMTP" is the only correct way to refer to
> libESMTP, I cannot be challenged, the directive is absolute. For example, the
> company "Rio Tinto-Zinc" really does have the hyphen between Tinto and Zinc.
> Or at least they did until they rebranded.

The only real exception to "libESMTP" is when systems insist on all lower case
for some reason.  Sometimes it is spelt with a capital "L", that's just wrong,
even at the beginning of a sentence, though I've made that error myself
sometimes.  Don't mock the name, I put maybe 25 seconds of thought into that.

## Reaction

I feel that libESMTP has always been a bit niche.  Most users seem to have just
quietly got on with it with nothing much to say, positive or negative.  An
early adopter was the Balsa email client, in part because I submitted the
initial patches and in part because it meant that, at least for a while, it had
better email submission than Netscape Communicator. A good number of early
contributions came from the Balsa project, especially in the TLS support.

A significant early contribution came from (if memory serves) Cambridge
University in the UK where libESMTP has been put through a static analysis
program in development there.  It had found only two issues, both potential
buffer overflows.  I'm told this was the best result from the open-source
projects they had analysed at the time!  That was quite a boost for my ego too.
More recently, the Clang static analyser gave it a clean bill of health.

Other than that, there was a small trickle of patches from other sources for
bugs that came to light but after a while libESMTP became quite stable and the
trickle of patches dried up completely.

Because libESMTP supported NTLM authentication with MS Exchange servers I
unexpectedly gained a reputation for being an expert in the area.  If I did it
was flattering but undeserved, all I did was search online until I found an
account of how to do NTLM and coded up something to match.  To my surprise, it
worked. It was mostly an issue of bit packing and endianness, Microsoft's brand
of Unicode and a weak MD4 based cipher.  Then again, NTLM caused people in the
open source world problems back then so maybe I *was* the expert.

I suspect my overall experience is typical for open source projects; people
tend to be more vocal when things do not work as expected.

What I was not prepared for was the abuse.

## Ad-Hominem

A lot of mail I received was not bug-fixes, critique, or discussion one might
expect but rude or abusive.  This was a surprise and deeply upsetting. Bear in
mind that I have never been paid for any of the work I put into libESMTP,
except when someone once sent me a £10 donation via PayPal less what ever their
commission was at the time.

I had put a lot of time into development and testing into libESMTP and it
worked fairly well and *it was free*.  I thought people might be grateful -
many were - but it certainly did not feel like that in 2001.  Nobody wants to
read through ad-hominem attacks to find the useful stuff, or that there was
none. I probably missed out on a lot of helpful advice because of that.  I
moved on to other things and put less effort into libESMTP than I might
otherwise have done.

If anyone has ever detected a somewhat, errmm..., stroppy tone in the bug
reporting section, that's due to a certain level of exasperation.  I received
many, often rude, messages about things which were explained on the old
web-site or where users' expectations were flatly contradicted by the RFCs.
After I updated the bug reporting page, those messages stopped coming.

When I think back from the on-line experience in 2020, all of this is much less
surprising and nowadays perhaps I'd just let it wash over me.  The abuse was a
small sample of what was to come on the yet-to-evolve social media phenomenon.
[Facebook](https://www.facebook.com/), [Twitter](https://www.facebook.com/) and
other social media sites are as much vehicles of trolling and misinformation as
they are of rational thought and discussion, or more so.

Since many projects have added a *code-of-conduct* to their web-sites I suspect
my experience is not unique.

## Hosting

Originally there was a libESMTP mailing list, but unfortunately this was before
the days of effective [spam](https://en.wikipedia.org/wiki/Spamming) filtering
and the mailing list quickly became overwhelmed.  I literally couldn't clean
the junk mail fast enough.  That fell by the wayside pretty early on.  With
hindsight I should probably have looked around for a host providing a
subscription based mailing list. So I used my personal email address for
libESMTP issues but this was not particularly satisfactory.

The libESMTP website continued to work well, however, and libESMTP had pretty
modest hosting requirements - basically just the web pages and a few tarballs.
Nevertheless, libESMTP has always suffered by not having a proper issue
tracking system.

## Documentation

I believe the libESMTP API is fairly logical and straightforward to use, but
any project relies on good documentation.  Originally I had the idea that the
API Reference should be available in different formats so I chose to write it
using [Docbook XML](https://docbook.org/) markup.  Without a purpose designed
tool, this turned out to be a world of hurt and pain.  There were, however,
XSL-T stylesheets available to translate the output to HTML and XSL-FO.  I was
never able to make use of the latter but, with a little customised XSL-T and
CSS, the HTML output looked good.

Because Docbook is difficult to write with a programmer's editor, the API
document progressed up to a point and stalled somewhat.  Although the majority
of public APIs were documented, quite a few remain to this day to be done and
none of the auth-client API was ever documented.  The example program
provided with libESMTP or reviewing the source was all there was for a good
portion of the API.  I can't say for sure if this was ever a show-stopping
problem but I really have no idea either way.

## Subsequent Development

The bulk of development on libESMTP was from about 2001 to 2006 with things
settling down after that; I was receiving few or no bug reports and the last
major release was about 2010.  I feel this is because libESMTP has been pretty
solid and reliable.

After about 2004, having changed employment I did not have the freedom to
pursue IETF and opensource activity I previously had enjoyed.  As development
work became more demanding and had shifted away from mail based technologies,
time and motivation to further develop libESMTP were in short supply. 

## Website

Sometime in the early 2010s my libESMTP email address stopped working, it was
mostly junk mail so I barely noticed.  After a few more years the website
disappeared too. To be fair I was probably notified but the registered email
address was long since obsolete.

I first found out about the disappearance at a job interview!  I had mentioned
libESMTP on my CV and as part of the prep work my interviewer had looked it up
and it was gone! Oops!  Fortunately libESMTP had been well established as part
of most Linux distributions by that time and it was incorporated by an open
source project organisation was already using. But it was an awkward moment.
To be fair I had had almost 15 years of free ISP services, so I'm not
complaining.  I should have moved to GitHub in its early days.  I got the job.

## Recent Work

Some years ago I set up the [GitHub](https://github.com/) account to host
libESMTP, but the lack of spare time and experience with Git (I had been using
CVS and SVN previously) meant that I more or less forgot about it, even when
Microsoft subsequently acquired GitHub.  However since then I had been using
[git](https://git-scm.com/) and [GitLab](https://about.gitlab.com/)
professionally so I decided to use these for personal projects too.

GitLab being a heavyweight for personal use I adopted [Gogs](https://gogs.io/)
instead.  Currently I run Gogs on a Raspberry Pi 2 with a RAID-10 array of USB
sticks, which sounds a bit crazy!  Sometimes it's a bit sluggish because the
RPi's USB and ethernet support is actually a 4-Port USB hub with a built-in
ethernet dongle, but this combination works extremely well and I don't have to
worry about energy usage.

## Modernisation

Late last year (2019) I decided to modernise the libESMTP distribution.  Some
time ago during a major move, I had found an old hard drive with all the
libESMTP tarballs back to version 0.8.  I was able to recreate a git repository
from the tarballs and used that as a starting point.  I probably still have the
old SVN repository but it isn't really worth the effort to migrate.  I've been
slowly updating things since then.

## SASL

Back in 2010 I added support for [GNU SASL](https://www.gnu.org/software/gsasl/)
as an alternative to libESMTP's native SASL implementation and released this as
version 1.0.7rc1.  I never had any feedback on this and I have no idea if
anyone is using it. It's currently in git on the gsasl branch.

## Autotools

A constant thorn in my side has been GNU autotools.  I hate autotools. I never
liked them, in part because of m4 quoting and in part because the generated
configure script was bigger than the code it was building, causing significant
bloat in the distribution. And all those commands that must be run in just the
right order with just the right options.  After a few updates to some of the
associated files I was finding it difficult to change things; I felt the
infrastructure was rather fragile, especially for out-of-tree builds or
cross-builds if things weren't set up just-so.  Also many projects were
migrating to [Meson](https://mesonbuild.com/) and I was losing the will to
live.  Enough was enough.

The first thing to do was migrate to Meson/[Ninja](https://ninja-build.org/).

## Obsolete Code

When libESMTP was originally written, getaddrinfo() had only recently been
introduced and was not widely supported.  Rather than polluting the source with
lots of `#ifdef`s I wrote a simple IPv4-only emulation round gethostbyname()
and a few other network functions.  These and a few other items are unnecessary
in contemporary systems and are removed. Other functions that supplemented a
local C library are removed or consolidated and there are a few changes to
ensure SUS/Posix compliance for things like header files and so on.

## Compliance

There are a few updates to RFCs in the intervening years. libESMTP needs review
of the code to update references to current documents and to ensure code
complies with anything that has changed.  Because of how interoperability and
changes are managed in RFC development this is likely to be a simple task.

## Unit Tests

I have added a branch for unit tests on some of the libESMTP internals.  I was
bitten by a major regression sometime around 2008 causing X.509 certificate
validation to fail.  The fix was simple enough but this sort of thing should
not happen.

## Documentation

Most of the recent work has been to gradually bring documentation up to modern
conventions. For example, facilities like Github pages or
[Read the Docs](https://readthedocs.org/).

I decided to move the API documentation into comments in the source, similar to
[gtkdoc](https://wiki.gnome.org/DocumentationProject/GtkDoc) conventions.  This
proved to be much harder to do than I expected.  Gtk-doc is a pretty flaky
toolchain in my experience. It's fine for GObject based programs but falls
apart quickly when presented with something similar to GObject code but not
quite - like libESMTP.

[HotDoc](https://hotdoc.github.io/) was much better but I still wasn't happy
with some aspects of the result. But HotDoc is promising, especially as a
gtkdoc replacement, and it stays firmly on my radar.

[Sphinx](https://www.sphinx-doc.org/) gives the results I was looking for,
especially with the Read the Docs theme.  The only problem is extracting the
documentation comments from the source.  I settled on using the kernel-doc
program from the [Linux Kernel](https://www.kernel.org/) distribution since it
does most of what I want.  The bits I don't like I can live with for now.

Migrating to Sphinx means a preference for ReStructuredText over Markdown.
Personally, I have a mild preference for Markdown as I find it easier to write
and the end result looks more like a plain text file than the former.  For the
documentation comments in the source code this makes little difference
but for other files like this one I just find Markdown easier.

## In Conclusion

I am hopeful libESMTP will continue to be useful in the future. libESMTP now
has a modern build system and the project can make better use of GitHub's
facilities.

I am extremely proud of libESMTP. Even after 20 years I believe it is
unparalleled as an SMTP client. Although it is in need of some modern
authentication mechanisms, the core library is robust and, I believe, despite
its age is still complies with modern standards.

And if you're ever stuck on the end of a limited bandwidth network or dial up
is your only option, libESMTP is the only show in town!  Well, I would say
that, wouldn't I?

----
Brian Stafford, June 2020.
