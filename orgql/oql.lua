-- This function simply takes an input, and should rewrite it into SQL.
-- It should return the transformed SQL query (such as "SELECT * FROM
-- todo WHERE heading LIKE '%blah%'"), a filelist (such as
-- "D:\todo.org,C:\blah.org"), and a suitable output (such as "CSV" or "pretty")
-- in a returned tuple.
-- Beware that I am basically a Lua beginner, so there is almost certainly
-- cleaner ways of doing everything below..

function parse(query)
	q = ""
	output = ""
	filelist = ""

	skipnext = false

	t = {}

	for v in string.gmatch(query, "([^ ]*)") do
		if v ~= "" then
			table.insert(t, v);
		end
	end

	for i=1,#t do
		uword = string.upper(t[i])

		if skipnext == false then
			if uword == "SELECT" or uword == "SHOW" then
				if string.upper(t[i + 1]) == "FROM" then
					q = q .. "SELECT * "
				else
					q = q .. "SELECT "
				end
			elseif uword == "UPDATE" or uword == "CHANGE" then
				filelist = t[i + 1]
				q = q .. "UPDATE todo "
				skipnext = true
			elseif uword == "INSERT" or uword == "ADD" then
				filelist = t[i + 1]
				q = q .. "INSERT INTO todo "
				skipnext = true
			elseif uword == "*" or uword == "ALL" or uword == "EVERYTHING" then
				output = "pretty"
				q = q .. "* "
			elseif uword == "FROM" then
				filelist = t[i + 1]
				q = q .. "FROM todo "
				skipnext = true
			elseif uword == "WITH" then
				q = q .. "WHERE "
			elseif uword == "MAKE" then
				q = q .. "SET "
			elseif uword == "HAS" or uword == "LIKE" or uword == "CONTAIN" or uword == "CONTAINS" or uword == "CONTAINING" then
				q = q .. "LIKE '%" .. t[i + 1] .. "%' "
				skipnext = true
			elseif uword == "IS" or uword == "==" or uword == "EQUALS" or uword == "BEING" or uword == "ARE" or uword == "TO" then
				q = q .. "= '" .. t[i + 1] .. "' "
				skipnext = true
			elseif uword == "WITHIN" then
				q = q .. string.format("BETWEEN strftime('%%s', 'now') AND strftime('%%s', 'now') + %i ", oql.parseTime(t[i + 1]))
				skipnext = true
			elseif uword == "IN" then
				q = q .. string.format("= strftime('%%s', 'now') + %i ", oql.parseTime(t[i + 1]))
				skipnext = true
			elseif uword == "SORT" then
				q = q .. "ORDER "
			elseif uword == "OUT" or uword == "OUTPUT" or uword == "FORMAT" then
				output = t[i + 1];
				skipnext = true
			else
				q = q .. t[i] .. " "
			end
		else
			skipnext = false
		end
	end

	return q, filelist, output
end
