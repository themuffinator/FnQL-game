# Licensing

## New FnQL-game code

Files written for FnQL-game carry a GPL header: GNU General Public License,
version 2 or (at your option) any later version. That matches the FnQL engine
and its Quake III Arena GPL lineage.

## Baseline code (open question)

The files imported from baseq3a keep their original headers: the id Software
copyright line with no license text. baseq3a ships id Software's *Limited Use
Software License Agreement* (the Quake III Arena game-source SDK license),
kept here as
[`baseline/QIIIA-Game-Source-License.txt`](baseline/QIIIA-Game-Source-License.txt).
GitHub detects no other license for the upstream repository.

What is known:

- id Software released the Quake III Arena 1.32 source, including this game
  code lineage, under GPLv2 in 2005. The id-authored portions are therefore
  available under GPLv2 from that release.
- baseq3a's own changes (by ec- and contributors) carry no separate license
  statement. The only license text in that repository is id's Limited Use
  agreement.

What is not settled: whether baseq3a's changes may be distributed under the
GPL alongside FnQL. Until that is confirmed with upstream:

- development and local builds are fine;
- binary distribution of FnQL-game (milestone 11) is blocked;
- if confirmation never comes, the alternative is to rebase onto the id GPL
  release and re-derive the baseq3a fixes independently.

## Retail content

FnQL-game ships no Quake Live or Quake III assets, binaries or data. Runtime
use requires a legitimate Steam install of Quake Live.
