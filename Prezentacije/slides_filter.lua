--[[
  Slajd koji sadrzi iskljucivo sliku (s opisom ili bez njega) automatski
  se oznacava kao "plain" -- beamer na njemu ne crta podnozje, pa slika
  moze zauzeti cijelu visinu slajda.

  Zahvaljujuci ovome u .md datoteci nema pandoc-specificnih atributa
  tipa {.plain}, pa se izvor uredno prikazuje i na GitHubu.
--]]

local function is_slika(blok)
  if blok.t == "Figure" then
    return true
  end
  if blok.t == "Para" and #blok.content == 1 and blok.content[1].t == "Image" then
    return true
  end
  if blok.t == "Plain" and #blok.content == 1 and blok.content[1].t == "Image" then
    return true
  end
  return false
end

function Pandoc(doc)
  local blokovi = doc.blocks
  local i = 1
  while i <= #blokovi do
    local blok = blokovi[i]
    if blok.t == "Header" and blok.level == 2 then
      -- sadrzaj slajda: sve do sljedeceg naslova razine 1 ili 2
      local j = i + 1
      local samo_slike = false
      local prazno = true
      while j <= #blokovi do
        local sljedeci = blokovi[j]
        if sljedeci.t == "Header" and sljedeci.level <= 2 then
          break
        end
        prazno = false
        if is_slika(sljedeci) then
          samo_slike = true
        else
          samo_slike = false
          break
        end
        j = j + 1
      end
      if samo_slike and not prazno then
        blok.classes:insert("plain")
      end
    end
    i = i + 1
  end
  return doc
end
