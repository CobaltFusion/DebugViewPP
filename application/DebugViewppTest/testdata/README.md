# Test data: files written by older versions of DebugView++

These files were written by the application **before boost was removed** (boost property_tree wrote the XML and JSON). Files
like these exist on the machines of users, so they must stay readable, whatever reads them. They are stored byte for byte
(see `.gitattributes`), do not reformat them, and do not regenerate them with newer code: the point is that they are old.

| File | What it is | How it was made |
|---|---|---|
| `session_input.dbconf` | A hand written session file (2 views, every filter type and match type, colors including the automatic one, XML special characters, a non-ASCII view name, columns, 2 disabled sources). It is the input to the next file. | written by hand, using the format of `CMainFrame::SaveConfiguration` |
| `session_saved_by_app.dbconf` | The same session after the application loaded it and saved it again with **File, Save Configuration**. This is what the application really writes. | `DebugViewpp.exe session_input.dbconf`, File / Save Configuration. The only edit: the name of the first view is `All` there, because loading a configuration does not rename the existing first view; it was changed to `Errors` so that the file does not depend on the settings of the machine it was made on. |
| `filters_legacy.xml` | A filter file as the filter dialog saved it as XML. | `SaveXml()` of `LogFilter.cpp` with the filters of the session above |
| `filters_legacy.json` | The same, saved as JSON. | `SaveJson()` of `LogFilter.cpp` |

Things to know when reading them:

- Line endings are CRLF, the text is UTF-8, boost wrote `&quot;` and `&apos;` as well as `&amp;`, `&lt;`, `&gt;`.
- A color that is automatic is an XML attribute: `<BackColor Auto="true"/>` (in JSON `"BackColor": {"<xmlattr>": {"Auto": "true"}}`).
- **The JSON is not valid JSON in the strict sense:** a list of filters is written as the key `"Filter"` repeated inside one object (property_tree has no arrays here), and
  every value is a string (`"true"`, `"255"`). A standard JSON parser keeps only the last `"Filter"`, so a reader of this file has to collect the duplicate keys.
- In `session_saved_by_app.dbconf` the order of a column is its position in the header (columns that are not shown are skipped), and a view without a `Columns` element in the input gets one when it is saved.
