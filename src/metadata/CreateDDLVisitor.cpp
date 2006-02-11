//-----------------------------------------------------------------------------
/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License") you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babauskov.

  Portions created by the original developer
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "CreateDDLVisitor.h"
//-----------------------------------------------------------------------------
CreateDDLVisitor::CreateDDLVisitor()
{
}
//-----------------------------------------------------------------------------
CreateDDLVisitor::~CreateDDLVisitor()
{
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getSQL() const
{
    return sqlM;
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getPrefixSql() const
{
    return preSqlM;
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getSuffixSql() const
{
    return postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Column&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Database&)
{
    // TODO: build the sql script for entire database?
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Domain& d)
{
    sqlM = wxT("CREATE DOMAIN ") + d.getQuotedName() + wxT("\n AS ") +
            d.getDatatypeAsString() + wxT("\n");
    wxString dflt = d.getDefault();
    if (!dflt.IsEmpty())
        sqlM += wxT(" DEFAULT ") + dflt + wxT("\n");
    if (!d.isNullable())
        sqlM += wxT(" NOT NULL\n")
    wxString check = d.getCheckConstraint();
    if (!check.IsEmpty())
        sqlM += wxT(" CHECK (") + check + wxT(")\n");
    wxString collate = d.getCollation();
    if (!collate.IsEmpty())
        sqlM += wxT(" COLLATE ") + collate;

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Exception& e)
{
    sqlM = wxT("CREATE EXCEPTION )" + e.getQuotedName() + wxT("'") +
        e.getMessage() + wxT("'\n");

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Function& f)
{
    sqlM = f.getCreateSQL();
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Generator& g)
{
    sqlM = wxT("CREATE GENERATOR ") + g.getQuotedName() + wxT("\n");
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Procedure& p)
{
    wxString temp(p.getAlterSql());
    postSqlM = temp;
    temp.Replace(wxT("ALTER"), wxT("CREATE"), false);   // just first
    sqlM = temp;

    // TODO: grant execute on [name] to [user/role]

    preSqlM = wxT("SET TERM ^ ;\nCREATE PROCEDURE ") + p.getQuotedName()
        + wxT(" ") + p.params
        + wxT("\nAS\nBEGIN\n/* nothing */\nEND^\nSET TERM ; ^");
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Parameter&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Role& r)
{
    sqlM = wxT("CREATE ROLE ") + r.getQuotedName();

    // TODO: grant role [name] to [user]

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Root&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Server&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Table& t)
{
    preSqlM = wxT("CREATE TABLE ") + t.getQuotedName() + wxT("\n(\n");
    for (MetadataCollection<Column>::iterator it=t.begin(); it!=t.end; ++it)
        preSqlM += (*it).getDDL();

    // primary keys (detect the name and use CONSTRAINT name PRIMARY KEY... or PRIMARY KEY(col)
    ColumnConstraint *pk = t.getPrimaryKey();
    if (pk)
    {
        presqlM += wxT(",");
        if (pk->getName_().StartsWith("INTEG_"))     // system one, noname
            presqlM += wxT("\n  PRIMARY KEY (");
        else
            presqlM += wxT("\n  CONSTRAINT ") + pk->getQuotedName() + wxT(" PRIMARY KEY (");

        for (std::vector<wxString>::const_iterator it = pk->begin(); it != pk->end(); ++it)
        {
            if (it != pk->begin())
                preSqlM += wxT(",");
            Identifier id(*it);
            preSqlM += id.getQuoted();
        }
        preSqlM += wxT(")");
    }

    // unique constraints
    std::vector<ColumnConstraint> *uc = t.getUniqueConstraints();
    if (uc)
    {
        for (std::vector<ColumnConstraint>::const_iterator ci = uc->begin(); ci != uc->end(); ++ci)
        {
            presqlM += wxT("\n  CONSTRAINT ") + (*ci).getQuotedName() + wxT(" UNIQUE (");
            for (std::vector<wxString>::const_iterator it = (*ci).begin(); it != (*ci).end(); ++it)
            {
                if (it != (*ci).begin())
                    preSqlM += wxT(",");
                Identifier id(*it);
                preSqlM += id.getQuoted();
            }
            preSqlM += wxT(")");
        }
    }

    // foreign keys
    std::vector<ForeignKey> *fk = getForeignKeys();
    if (fk)
    {
        for (std::vector<ForeignKey>::const_iterator ci = fk->begin(); ci != fk->end(); ++ci)
        {
            Identifier reftab((*ci).referencedTableM);
            wxString src_col, dest_col;
            for (std::vector<wxString>::const_iterator it = (*ci).begin(); it != (*ci).end(); ++it)
            {
                if (it != (*ci).begin())
                    src_col += wxT(",");
                Identifier id(*ci);
                src_col += id.getQuoted();
            }
            for (std::vector<wxString>::const_iterator it = (*ci).referencedColumnsM.begin();
                it != (*ci).referencedColumnsM.end(); ++it)
            {
                if (it != (*ci).begin())
                    dest_col += wxT(",");
                Identifier id(*ci);
                dest_col += id.getQuoted();
            }
            postSqlM += wxT("ALTER TABLE ") + t.getQuotedName() + wxT(" ADD CONSTRAINT ") +
                (*ci).getQuotedName() + wxT(" FOREIGN KEY (") + src_col +
                wxT(" REFERENCES ") + reftab.getQuoted() + wxT(" (") + dest_col + wxT(");\n");
        }
    }

    // check constraints
    std::vector<CheckConstraint> *chk = t.getCheckConstraints();
    if (chk)
    {
        for (std::vector<CheckConstraint>::const_iterator ci = chk->begin(); ci != chk->end(); ++ci)
        {
            postSqlM += wxT("ALTER TABLE ") + t.getQuotedName() + wxT(" ADD CONSTRAINT ") +
                (*ci).getQuotedName() + wxT(" CHECK (") + (*ci).sourceM + wxT(");\n");
        }
    }

    // indices
    std::vector<Index> *ix = getIndices();
    if (ix)
    {
        for (std::vector<Index>::const_iterator ci = ix->begin(); ci != ix->end(); ++ci)
        {
            postSqlM += wxT("CREATE ");
            if ((*ci).getIndexType() == Index::itDescending)
                postSqlM += wx("DESCENDING ");
            postSqlM += wxT("INDEX ") + (*ci).getQuotedName() + wxT(" ON ")
                + t.getQuotedName() + wxT(" (");
            std::vector<wxString> *cols = (*ci).getSegments();
            for (std::vector<wxString>::const_iterator it = cols->begin(); it != cols->end(); ++it)
            {
                if (it != cols->begin())
                    postSqlM += wxT(",");
                Identifier id(*it);
                postSqlM += id.getQuoted();
            }
            postSqlM += wxT(");\n");
        }
    }

    // TODO: grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    //postSqlM +=

    preSqlM += wxT("\n);\n");
    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Trigger& t)
{
    wxString object, source, type, relation;
    bool active;
    int position;
    t.getTriggerInfo(object, active, position, type);
    t.getSource(source);
    t.getRelation(relation);

    sqlM << wxT("SET TERM ^ ;\nCREATE TRIGGER ") << t.getQuotedName()
         << wxT(" FOR ") << relation;
    if (active)
        sqlM << wxT(" ACTIVE\n");
    else
        sqlM << wxT(" INACTIVE\n");
    sqlM << type;
    sqlM << wxT(" POSITION ");
    sqlM << position << wxT("\n");
    sqlM << source;
    sqlM << wxT("^\nSET TERM ; ^");

    postSqlM = sqlM;    // create triggers at the end
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(View& v)
{
    wxString src;
    v.checkAndLoadColumns();
    v.getSource(src);

    sqlM = wxT("CREATE VIEW ") + getQuotedName() + wxT(" (");
    bool first = true;
    for (MetadataCollection<Column>::const_iterator it = v.begin();
        it != v.end(); ++it)
    {
        if (first)
            first = false;
        else
            sqlM += wxT(", ");
        sqlM += (*it).getQuotedName();
    }
    sqlM += wxT(")\nAS ");
    sqlM += src;

    // TODO: grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(MetadataItem&)
{
    // empty
}
//-----------------------------------------------------------------------------
