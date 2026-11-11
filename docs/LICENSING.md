# Licensing

FnQL-game contains code under two different licenses, because it comes from
two sources. Each source file's header shows which group it belongs to.

## FnQL-game's own code: GPL

Files written for FnQL-game carry a GPL header: GNU General Public License,
version 2 or (at your option) any later version. That matches the FnQL engine
and its Quake III Arena GPL lineage.

## Baseline code: id Software's game-source license

The files imported from baseq3a keep their original headers: the id Software
copyright line with no license text. baseq3a ships id Software's *Limited Use
Software License Agreement* (the Quake III Arena game-source SDK license),
kept here as
[`baseline/QIIIA-Game-Source-License.txt`](baseline/QIIIA-Game-Source-License.txt).
GitHub detects no other license for the upstream repository.

What is known:

- id Software released the Quake III Arena 1.32 source, including this game
  code lineage, under GPLv2 in 2005. The id-authored portions are therefore
  also available under GPLv2 from that release.
- baseq3a's own changes (by ec- and contributors) carry no separate license
  statement. The only license text in that repository is id's Limited Use
  agreement.

What is not settled: whether baseq3a's changes may be relicensed under the
GPL. Upstream has not confirmed it. If confirmation never comes, the fallback
is to rebase onto id's GPL release and re-derive the baseq3a fixes
independently, which would leave FnQL-game GPL-only.

## Binary distribution

FnQL releases bundle the modules built from a pinned commit of this
repository (`fnql_qagame`, `fnql_cgame` and `fnql_ui`). Every module links
code from both groups above, so the same binary is covered by both licenses:

- FnQL-game's own code is GPLv2 or later. Its source is this repository at
  the commit each release records in `docs/fnql-game/BUILD-INFO.txt`.
- The id-authored baseline code is available under GPLv2 through id's 2005
  release, and baseq3a publishes it under id's Limited Use agreement.
- baseq3a's changes are distributed on the only terms baseq3a states for
  them: id's Limited Use agreement.

Because baseq3a's changes are not confirmed GPL, the bundled modules are not
offered as purely GPL works. FnQL releases therefore ship id's agreement next
to the modules and distribute them only free of charge, for non-commercial
use, by electronic download, as that agreement requires. This document
records the project's understanding. It is not legal advice and does not
settle the open question above.

## Retail content

FnQL-game ships no Quake Live or Quake III assets, binaries or data. Runtime
use requires a legitimate Steam install of Quake Live.
