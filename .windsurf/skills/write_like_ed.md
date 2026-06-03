# Writing Style: Ed Hartnett

Rules for writing technical prose in Ed's voice, derived from his own sections of the NetCDF Developer's Handbook (chapters and narrative prose; excludes the foreword written by John Caron).

---

## Voice and Tone

- **Plain and direct.** No throat-clearing, no filler phrases. Get to the point immediately.
- **Confident but not arrogant.** State facts without hedging. Don't pad assertions with "perhaps" or "it could be said."
- **No sentimentality.** Technical history is told factually, not nostalgically.
- **First person is used sparingly and naturally.** When crediting yourself or describing your own work, use "I" without self-aggrandizement. Example: "In 2025 I released version 1.0 of the NetCDF Expansion Pack." Or: "In 2012, I split netcdf-fortran from netcdf-c, allowing independent evolution."
- **No passive-voice hedging.** Prefer active voice. "NetCDF solved three problems at once" not "three problems were solved."

---

## Sentence Structure

- **Short to medium sentences dominate.** Long sentences appear only when listing related items or building a logical chain. They are never meandering.
- **Parallel structure for lists.** When multiple things are described, the grammatical form is consistent across items. Example: "Its data model was simple... Its files were machine-independent... And its metadata was self-describing."
- **Colons introduce amplification or itemization.** Used cleanly, not overused.
- **No run-ons.** If a sentence contains more than two independent clauses, split it.
- **Transitions are minimal.** Ideas connect through logic and sequence, not through "Furthermore," "Moreover," or "Additionally."

---

## Paragraph Structure

- **One idea per paragraph.** Paragraphs are short and focused. Rarely more than 4-5 sentences.
- **Lead with the point.** The first sentence states the main idea; the rest support or illustrate it. Example opening: "NetCDF has no marketing department, does no advertising, and makes no effort to recruit new users."
- **Historical narrative is linear and attributed.** Events appear in chronological order with specific dates, version numbers, and named contributors.
- **Cause-and-effect is explicit.** Don't let the reader guess why something happened or why it matters.
- **Close on consequence.** Paragraphs often end with why something matters, not a restatement of what was done. Example: "That interoperability is the defining feature of netCDF."

---

## Technical Writing Conventions

- **Introduce concepts before code.** A sentence or two of context precedes any code block.
- **Name functions, constants, and flags exactly.** Use the actual symbol names (`nc_create()`, `NC_CLOBBER`, `nf90_noerr`) inline in prose when referring to API elements.
- **State the consequence, not just the mechanic.** Don't just describe what a function does; say what it means. Example: "Closing the file releases all internal resources associated with the open file."
- **Cross-references are specific.** Point to the exact chapter, not "see later" or "elsewhere." Example: "For working with reanalysis datasets and CF-compliant satellite records, see Chapter 9 (*Attributes and Conventions*)."
- **Numbers in prose are numerals when technical** (version numbers, sizes, counts, dates). Example: "approximately 2 TB of data per day", "31 km resolution", "over 20 petabytes."
- **Constraints and trade-offs are named plainly.** "The original classic formats hit a wall at 2 GB." Not "there were some limitations."

---

## What Ed Does NOT Do

- No exclamation points in prose (they appear only in program output strings like `*** SUCCESS`).
- No marketing language: "powerful," "robust," "seamless," "cutting-edge," "state-of-the-art." The one exception is descriptive bullets for NEP features aimed at a promotional audience — but even then the language stays functional.
- No hedging chains: "it may be possible that," "one might consider," "in some cases."
- No restating the question before answering it.
- No sycophantic acknowledgment of the reader: "Great question," "As you may know," "Hopefully this helps."
- No vague numbers. Don't write "many petabytes" if "100 petabytes" is known.

---

## Structural Patterns

- **Key Takeaways sections** use bold lead-in phrases followed by a colon and a plain-English summary. Each item is one or two tight sentences. Example: "**Built for scale:** NetCDF-4 removed the classic format's size limits and added compression, chunking, and parallel I/O..."
- **Tables are used for comparisons and decision guides**, not padding. Columns are labeled precisely with concrete criteria.
- **Section headers are noun phrases or direct questions.** "Why Does NetCDF Exist?" "A Note on Multiple Methods." Not gerunds: prefer "What is NetCDF?" over "Understanding NetCDF."
- **Reader Roadmap pattern.** For navigational sections, a flat "If you want X: Chapter N" list is preferred over prose summaries.
- **Warnings and notes are embedded in prose** or in a brief direct statement, not buried in footnotes.

---

## Historical and Attribution Style

- **Credit people by full name the first time**, then by last name or role. Always name the institution when relevant.
- **Version numbers and years anchor every historical claim.** "Version 4.0 (2008) was a transformative release, introducing the enhanced data model built on HDF5..."
- **Explain why a release mattered**, not just what it contained. "It maintained backward compatibility while becoming the preferred format for complex datasets."
- **Self-reference is matter-of-fact, never boastful.** "About this time, I joined as co-developer." Attribution is factual, not promotional.

---

## Characteristic Sentence Patterns

- **Problem → solution framing.** State the problem plainly, then what was done: "Before common data formats like netCDF, every instrument and model produced data in its own format... NetCDF solved three problems at once."
- **Compressed sweep.** Summarize an entire era in one or two sentences: "PnetCDF and PIO address this today, but the demands keep growing."
- **Concrete contrast.** "Local disk access is fast and cheap per operation. Cloud object storage is slow per request but scales without limit."
- **Closing with stakes.** End sections on what is at risk or what is gained: "The future of netCDF lies with those scientists and their data needs."
- **"That" to crystallize.** Use "That X is Y" to nail down a conclusion: "That interoperability is the defining feature of netCDF." "That mattered when labs routinely exchanged data across incompatible systems."

---

## Capitalization

- `netCDF` (lowercase n, uppercase CDF) when used as a general noun or referring to the ecosystem mid-sentence.
- `NetCDF` (uppercase N) when beginning a sentence or in a proper title or product name.
- HDF5, HDF4, GRIB, GRIB2, GeoTIFF — always capitalized as proper nouns.
- API function names are always in the exact case of the source code.
