+++
draft: true
title: The FOSS Economic failure
+++

> Thesis: The FOSS model of software mistakes user agency for distribution freedom, this destroys the economic incentives for making high quality software.

# Introduction

Firstly understand what FOSS and OSS are. FOSS or "Free and open source" aka "libre" software is software in which a particular set of freedoms as defined by the FSF (free software foundation) are guaranteed by its license agreement.

< add freedoms here >

The adjacent term being "Open source software" typically defined by the Open Source Initiative does not use these four freedoms explicitly, however, it does effectivelly guarantee them. Examples of free software licenses include: MIT, BSD, GPL, Apache, MPL, etc.

Throughout this article the term FOSS shall be used to refer to these licensing models, it is important to keep in mind however that the FSF and the OSI have distinct philosophies. The FSF is significantly more activist and its four freedoms are part of its larger movement for user agency and distribution freedom. Meanwhile the OSI is more corporate and pragmatic, typically "selling" open source to companies as a way to build a solid auditable foundation with community support.

# Copyright and Skin in the Game

There's much to criticize about IP (Intellectual property) laws and copyright (a subset of IP laws). This article is not for that. The copyright system is enforced by most countries on earth, so as of now, it is a reality that might change someday but nonetheless it's here.

Copyright is not property rights, despite the terrible name of it being "intellectual property", there's no real property. Copyright instead is the "right to copy" quite literally, it is what is called an "honor system" in legal terms. It is an artificial monopoly that effectively says: "The author of a work has a right to _exclude_ others from copying it".

Unlike regular property rights, which have physical barriers around them that more clearly delineate to whom it belongs, ideas don't have that, there's no "natural impedance" to ideas such as a fence to jump over, an object to lift, there's only a completely negligible cost of making a copy.

The common defense of copyright (ignoring its misuses and how outrageously long it is stretched in modern legislation) is quite simple:

1. Creating good original work is difficult
2. Copying original work is trivial to the point of being able to easily "kill off" the original author in the market
3. To curb these incentives, an artifical right to exclusion to copy is given to the author, for a particular amount of time

Thus, copyright is a double edge sword, it is used to put "skin in the game" of those who did contribute to its creation, although it can also be used for rent-seeking behavior (that's why a time limit must exist).

Software is particularly volatile to copyright, if the cost of copying books is quite small to writing one, the cost of copying software may as well be just a couple of Watts and some memory.

Because writing good software is difficult (yes, even with LLMs) the copyright system serves as a legal barrier to allow for software developers to make a living, in the same way authors and artists do.

Ultimately, there is no perfect legal system, and it is a matter of balancing tradeoffs and incentives. Human nature is always going to be corrupt, and currently copyright is the legal device to prevent "stealing" from original authors.

# Giving it for free!!!

FOSS effectively uses the copyright system against itself, because copyright allows the author to add licensing terms, just as one can make a contract to lease a particular physical property. By making the licensing term something like this:

< The MIT license >

The author "gives away" his rights. Legally speaking, the author does still retain the copyright, but it is _licensed_ for extremely generous conditions like "just give me credit, and don't sue me". That by itself is not wrong in the slightest but I want to point out something very important here, what the author is doing here is _charity_, this is a _gift_ and certainly not a viable business model.

Disclosing the source code by itself can already reveal trade secrets which can be the difference between having a business or not, sharing the source code couples that to effectively forsaking the right to enforce anything on the source artifact.

This creates a serious issue, companies are incentivized to get code from open source while the individual creators are made sitting ducks in terms of income. Licenses like the GPL go a step further in trying to force you to do charity, which makes them unusable in any commercial product.

Companies can also trivially violate the GPL, the authors basically make no money anyways so who will pay the lawyers? The FSF? The FSC? How will you appeal to them? How can they persecute hundreds of companies at once? FOSS puts creators into a tough spot, even if people disrespect the generous conditions offered, you basically have nothing to persecute them with.

FOSS also indirectly incentivizes people to just not pay, it is supposedly "Free as in Free Speech" but in practice it is always "Free as in beer".

# "Ok, corporate bootlicker!"

It is very important to understand also the other side of the coin, there are really abusive EULAs that put the user in an abusive position. Other restriction mechanisms like DRM and disguised "security" features take away user agency, very often from a real physical device they bought.

However, the solution to these companies is more high quality software and potentially legal mechanisms to protect the consumer, something that FOSS has trouble making due to the afformentioned bad incentives to have the money and influence required for that.

A fatal flaw of the FOSS movement is that it conflates a lot of things, the distribution of trade secrets (source code), the right to distribute copyrighted material and user agency. I'm a firm believer in higher user agency, but this has to be counter-balanced by commercial viability and not allowing people with zero skin in the game to effectively re-sell your work.

To see how user agency and source availability are usually orthogonal:

| Software       | FOSS | User respecting         |
| -------------- | ---- | ----------------------- |
| Sublime Text   | No   | Yes                     |
| Reaper         | No   | Yes                     |
| Linux Kernel   | Yes  | Yes                     |
| Android (AOSP) | Yes  | No (worse then windows) |
| Office Suite   | No   | No                      |

# I still like FOSS

Very important to mention, charity isn't bad. I like FOSS. I make FOSS from time to time, and there are definetly cases where openness is very curcial, such as in file formats, network protocols and cryptographic standards. What I'm specifically against is the normalization of unpaid labor of developers, this atittude enabled large exploitation of charity without the due reciprocity of the gift economy (see: LLM training, AWS, Microsoft, Google).
