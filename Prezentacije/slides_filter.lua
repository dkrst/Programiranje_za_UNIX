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

-- Naslov poglavlja ispred kojega stoji HTML komentar <!-- bez-medjuslajda -->
-- pretvara se u \section*, cime beamer ne generira medjuslajd s naslovom
-- poglavlja. Komentar je nevidljiv u prikazu .md datoteke na GitHubu.
local function bez_medjuslajda(blokovi)
  local izlaz = {}
  local preskoci = false
  for i, blok in ipairs(blokovi) do
    if blok.t == "RawBlock" and blok.format == "html"
       and blok.text:find("bez%-medjuslajda") then
      preskoci = true
    elseif preskoci and blok.t == "Header" and blok.level == 1 then
      table.insert(izlaz, pandoc.RawBlock("latex", "\\global\\bezmedjuslajdatrue"))
      table.insert(izlaz, blok)
      preskoci = false
    else
      if preskoci then preskoci = false end
      table.insert(izlaz, blok)
    end
  end
  return izlaz
end

-- Blok koda ispred kojega stoji HTML komentar <!-- deklaracija -->
-- uokviruje se (LaTeX okruzenje "deklaracija" definirano u fesb_slides.tex).
-- Komentar je nevidljiv u prikazu .md datoteke na GitHubu.
local function uokviri_deklaracije(blokovi)
  local izlaz = {}
  local oznaceno = false
  for _, blok in ipairs(blokovi) do
    if blok.t == "RawBlock" and blok.format == "html"
       and blok.text:find("deklaracija") then
      oznaceno = true
    elseif oznaceno and blok.t == "CodeBlock" then
      table.insert(izlaz, pandoc.RawBlock("latex", "\\begin{deklaracija}"))
      table.insert(izlaz, blok)
      table.insert(izlaz, pandoc.RawBlock("latex", "\\end{deklaracija}"))
      oznaceno = false
    else
      if oznaceno then oznaceno = false end
      table.insert(izlaz, blok)
    end
  end
  return izlaz
end

function Pandoc(doc)
  doc.blocks = bez_medjuslajda(doc.blocks)
  doc.blocks = uokviri_deklaracije(doc.blocks)
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
