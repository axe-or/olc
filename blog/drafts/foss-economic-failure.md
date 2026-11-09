+++
draft: true
title: The Free and Open Source confusion
+++

The free software movement's "four freedoms" conflate the user's control over their own copy, and
the right to distribution. Treating them as a single moral claim has had far-reaching consequences
for the software industry. It normalized giving away the very rights that let small teams make a
living from high-quality software.

# Definitions

FOSS or "Free and open source" aka "libre" software is software in which a particular set of
freedoms as defined by the FSF (free software foundation) are guaranteed by its license agreement.
OSS is nearly identical, however the criteria are defined by the OSI (open source initiative).

Throughout this article the term FOSS shall be used to refer to these licensing models, it is
important to keep in mind however that the FSF and the OSI have distinct philosophies. The FSF is
significantly more activist and its four freedoms are part of its larger movement for user agency
and distribution freedom. Meanwhile the OSI is more corporate and pragmatic, typically "selling"
open source to companies as a way to build a solid auditable foundation with community support.
# The 4 freedoms

| Freedom                                  | What it grants                           | Category         |
| ---------------------------------------- | ---------------------------------------- | ---------------- |
| 0: run the program for any purpose       | Control over your own copy               | **Agency**       |
| 1: study and modify it (requires source) | Understanding and adapting your own copy | **Agency**       |
| 2: redistribute copies                   | Handing copies to others                 | **Distribution** |
| 3: distribute modified versions          | Handing your changes to others           | **Distribution** |

Only freedoms 0 and 1 concern what a user can do with _their own copy_. Freedoms 2 and 3 concern
what the user can do _for third parties_. They turn every customer into a potential distributor of
the product. Nothing about ensuring the user properly "own" their copy requires the second pair. A
license can grant full control over your copy, including the source, while reserving the right to
distribute it. Another thing is that freedom 1 does not require source at all, reverse engineering
exists.

The fatal flaw of the FOSS movement is that it treats three separate things as one: disclosing the
source code, granting the right to redistribute it, and respecting user agency. I firmly believe in
user agency. But it can be had without the second item, and the second item is what makes the work
commercially unviable. It lets people with zero skin in the game re-sell or give away what you made.

# Copyright and Skin in the Game

To see why distribution rights matter so much, we need to look at what copyright actually protects.

Unlike regular property rights, which have physical barriers around them that more clearly delineate
to whom it belongs, ideas don't have that, there's no "natural impedance" to ideas such as a fence
to jump over, an object to lift, there's only a negligible cost of making a copy.
The common defense of copyright (ignoring its misuses and how outrageously long it is stretched in modern legislation) is quite simple:

1. Creating good original work is difficult

2. Copying original work is trivial to the point of being able to easily "kill off" the original author in the market

3. To curb these incentives, an artificial exclusion right to copy is given to the author, for a particular amount of time

In effect, copyright extends the mechanisms of private property (the rights to sell, lease, or
license, and to inherit) to an _abstract form_: a story, a melody, a program. That makes it
inherently fuzzier than ownership of a physical thing. Strictly, copyright protects the expression
rather than the idea itself.
Thus, copyright is a double-edged sword, in one way it is used to put "skin in the game" of those
who contribute the creation in question, although it can also be used for rent-seeking behavior
(that's why a time limit must exist).

Software is particularly sensitive to this, the cost of copying books is quite small to writing one
but still requires a printer and ink, the cost of copying software may as well be just a couple of
Watts and some memory.

Because writing good software is difficult (yes, even with LLMs) the copyright system serves as a
legal barrier to allow for software developers to make a living, in the same way authors and artists
do.

# Giving it for free!!!

FOSS effectively uses the copyright system against itself, because copyright allows the author to
add licensing terms, just as one can make a contract to lease a particular physical property. By
making the licensing term something like this:

> Copyright <YEAR> <COPYRIGHT HOLDER>
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
> associated documentation files (the “Software”), to deal in the Software without restriction,
> including without limitation the rights to use, copy, modify, merge, publish, distribute,
> sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or
> substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
> NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
> NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
> OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
> CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Looking at the list of granted rights, we can see that only "use" and "modify" are actually relevant for a customer wanting to exert agency over the product he aquired.

The author "gives away" his rights. Legally speaking, the author does still retain the copyright,
but it is _licensed_ for extremely generous conditions like "just give me credit, and don't sue me".
That by itself is not wrong in the slightest but I want to point out something very important here,
what the author is doing here is _charity_, this is a _gift_ and certainly not a viable business
model.

Disclosing the source code has an inherent cost in revealing competitive advantages and the know-how
which can be the difference between having a business or not. FOSS adds another massive cost, it
couples forsaking the right to enforce the distribution rights of the source artifact. However, it
does not have to be this way, nothing requires that these two rights be coupled and leased at once
to the user.

Copyleft licenses like the GPL don't fix this, because they protect the wrong thing. The GPL
protects the _commons_: it ensures derivative work stays open. It does nothing for the _author's
income_, thus making enforcement weak. Since the author typically earns nothing from the work,
there's no time and money for lawyers. Violations go unchallenged unless an organization like the
Software Freedom Conservancy (SFC) takes up the case, which is a slow, expensive process funded by
charity.

FOSS also indirectly incentivizes people to just not pay, it is supposedly "Free as in Free Speech"
but in practice it is mostly "Free as in beer" because there's no actual incentive to buy outside of
a pure act of charity for the author.

# Companies that "sell" free software

The usual counterargument is that companies like Red Hat do make money from free software. Look
closer at what they're selling: Red Hat sells maintenance, certification, support, and cloud
services around a linux distribution.

The code itself can't be sold, so the business sells something else. That model also only works at a
scale small teams can't reach. And it rewards software that needs a lot of support. This is so clear
that Red Hat eventually restricted public access to RHEL's source in 2023, using subscription terms
to discourage redistribution, even though the GPL itself permits it.

Dual licensing like seen in MySQL, Qt is even more telling. The author holds the copyright, releases
the code under the GPL, and sells a commercial license to anyone who wants to ship it in a
proprietary product. That commercial license is an **exemption from the GPL's distribution terms**.
The revenue comes from selling back exactly the distribution rights that the free license gave away.
Dual licensing only works because it admits that money resides precisely in freedoms 2 and 3.

# "Ok, corporate bootlicker!"

It is very important to understand also the other side of the coin, there are really abusive EULAs
that put the user in an abusive position. Other restriction mechanisms like DRM and disguised
"security" features take away user agency, very often from a real physical device they bought.

The answer to these companies is to compete with them: more high-quality software that respects user
agency. Something that FOSS has trouble making due to the aforementioned bad incentives to have the
money and influence required for that. This can be done by better licensing options that don't
require completely sabotaging the commercial viability of the product.

User agency and source availability are largely orthogonal. Here, "user respecting" means software
that doesn't force telemetry, impose strict DRM, require being online, or try to stop users from
changing their own general-purpose computing device:

| Software             | Source Availability | User respecting? |
| -------------------- | ------------------- | ---------------- |
| Sublime Text         | Closed              | Yes              |
| Reaper               | Closed              | Yes              |
| RemedyBG             | Closed              | Yes              |
| Aseprite             | Available           | Yes              |
| Grayjay              | Available           | Yes              |
| Linux Kernel         | FOSS                | Yes              |
| Android (AOSP + GMS) | FOSS core + Closed  | No               |
| macOS, iOS           | Closed              | No               |
| MS Office            | Closed              | No               |

Closed software can still respect its users, and open software can be shipped in a form that
doesn't. Source availability shows up on both sides, too. What decides whether users are respected
is how the *vendor behaves*, not whether everyone is allowed to redistribute the code.

# I still like FOSS

Charity isn't bad. I like FOSS, and I release FOSS from time to time. There are cases where openness
is essential: file formats, network protocols, and cryptographic standards all depend on it. What I
oppose is treating unpaid developer labor as the default. A gift economy runs on reciprocity: a gift
creates an obligation to give something back. FOSS is a gift economy in which the largest recipients
(cloud providers, big tech, and now LLM training) take far more than they return. That imbalance is
the subject of a future article here.

The fix is **not** to abandon user agency but to stop treating it as inseparable from distribution
rights.
