sub tab_create($)
{ @t=(split(":", $_[0]));
  my $sql="CREATE DATABASE $EWB_sqldbase";

  sqlrun($sql);
  my $sql="CREATE TABLE $t[0](";
  for ($i=1; $i<=$#t; $i++)
  { $sql.="$t[$i] char(255),";
  };
  $sql=substr($sql, 0, length($sql)-1);
  $sql.=")";
  sqlrun($sql);
};
