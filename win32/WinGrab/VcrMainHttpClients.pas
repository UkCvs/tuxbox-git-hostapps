////////////////////////////////////////////////////////////////////////////////
//
// $Log: VcrMainHttpClients.pas,v $
// Revision 1.1  2004/07/02 14:02:37  thotto
// initial
//
//

////////////////////////////////////////////////////////////////////////////////
//
//  HTTP-Clients to receive Infos from DBox ...
//
function  TfrmMain.SetHttpHeader: String;
var
  sTmp : String;
begin
  sTmp:= ' ';
  sTmp:= sTmp + 'HTTP/1.0' + #13#10;
  sTmp:= sTmp + 'User-Agent: BS' + #13#10;
  sTmp:= sTmp + 'Host: '+ m_sDBoxIp + #13#10;
  sTmp:= sTmp + 'Pragma: no-cache' + #13#10;
  sTmp:= sTmp + 'Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*' + #13#10#13#10;
  Result := sTmp;
end;

function  TfrmMain.SendHttpCommand(sCommand: String): String;
var
  sTmp : String;
begin
  m_Trace.DBMSG(TRACE_CALLSTACK, '> SendHttpCommand');
  if m_bHttpInProgress then
  begin
    m_Trace.DBMSG(TRACE_SYNC, 'cancel SendHttpCommand ['+sCommand+']');
    m_Trace.DBMSG(TRACE_CALLSTACK, '< SendHttpCommand');
    exit;
  end;
  if not PingDBox(m_sDBoxIp) then
  begin
    m_Trace.DBMSG(TRACE_CALLSTACK, '< SendHttpCommand');
    exit;
  end;
  try
    try
      m_Trace.DBMSG(TRACE_SYNC, 'SendHttpCommand ['+sCommand+']');
      Result := '';
      m_bHttpInProgress := true;
      VcrEpgClientSocket.Active:= false;
      VcrEpgClientSocket.Port   := 80;
      VcrEpgClientSocket.Address:= m_sDBoxIp;
      Application.ProcessMessages;
      VcrEpgClientSocket.Active:= true;
      DoEvents;
      VcrEpgClientSocket.Socket.SendText('GET ' + sCommand + SetHttpHeader);
      DoEvents;
      Sleep(1000);
      DoEvents;
      sTmp := VcrEpgClientSocket.Socket.ReceiveText;
      m_Trace.DBMSG(TRACE_SYNC, 'Response ='+#13#10+'['+sTmp+']'+#13#10);
      if Length(sTmp) > 0 then
      begin
        if Pos(#13#10#13#10, sTmp) > 1 then
        begin
          if Pos('OK', sTmp) > 1 then
          begin
            sTmp := Copy(sTmp, Pos(#13#10#13#10, sTmp)+4, Length(sTmp)-4);
          end else
          begin
            sTmp := '';
          end;
        end;
      end;
      Result := sTmp;
      m_bHttpInProgress := false;
    except
      on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'SendHttpCommand '+E.Message);
    end;
  finally
    m_bHttpInProgress := false;
  end;
  m_bHttpInProgress := false;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< SendHttpCommand');
end;

procedure TfrmMain.GetProgEpg(sEventId,
                              sFilePath: String;
                              var epgtitel,
                              epgsubtitel: String);
begin
  m_Trace.DBMSG(TRACE_CALLSTACK, '> GetProgEpg');
  try
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< GetProgEpg');
      exit;
    end;
    DoEvents;
    //save EPG-File if needed ...
    if sFilePath <> '' then
    begin
      SaveEpgText( sEventId, sFilePath, epgtitel, epgsubtitel ).Free;
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'GetProgEpg '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< GetProgEpg');
end;

function TfrmMain.SaveEpgText(aUrl,
                              sFilePath: String;
                              var epgtitel,
                              epgsubtitel: String): TStringList;
var
  sDbg : String;
  sTmp : String;
  i    : Integer;
begin
  m_Trace.DBMSG(TRACE_CALLSTACK, '> SaveEpgText');
  epgsubtitel := '';
  Result := TStringList.Create;
  try
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< SaveEpgText');
      exit;
    end;

    sDbg := 'SaveEpgText eventid=' + aUrl;
    m_Trace.DBMSG(TRACE_SYNC, sDbg);

    sTmp   := SendHttpCommand('/fb/epg.dbox2?eventid=' + aUrl);
    if Length(sTmp) > 0 then
    begin
      Result.Text := sTmp;
      i:= Pos('<b>',LowerCase(sTmp));
      if i <> 0 then
      begin
        sTmp := Copy( sTmp, Pos('<b>',LowerCase(sTmp))+3, Length(sTmp));
        sTmp := Trim(Copy( sTmp, 1, Pos('</b>',LowerCase(sTmp))-1));
        if sTmp <> 'keine ausf�hrlichen Informationen verf�gbar' then
        begin
          epgsubtitel := sTmp;
        end;
        if Length(sFilePath) > 0 then
        begin
          Result.SaveToFile(sFilePath + CheckFileNameString(epgtitel) + '_-_' + CheckFileNameString(epgsubtitel) + '.HTML');
        end;
      end;
    end;
    DoEvents;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'SaveEpgText '+ E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< SaveEpgText');
end;

procedure TfrmMain.GetChannelProg(    ChannelId: String;
                                  var epgid: integer;
                                  var epgtitel: string);
var
  sDbg     : String;
  sTmp     : String;
  sLine    : String;
  sEventId : String;
  sFstEvId : String;
  sZeit    : String;
  sDatum   : String;
  sUhrzeit : String;
  sDauer   : String;
  sTitel   : String;
  sSubTitel: String;
  sEpg     : String;
  Datum    : TDateTime;
begin
  m_Trace.DBMSG(TRACE_CALLSTACK, '> GetChannelProg ' + ChannelId);
  if IsDBoxRecording then
  begin
    m_Trace.DBMSG(TRACE_CALLSTACK, '< GetChannelProg (DBox is recording now)');
    exit;
  end;
  if not PingDBox(m_sDBoxIp) then
  begin
    m_Trace.DBMSG(TRACE_CALLSTACK, '< GetChannelProg');
    exit;
  end;
  try
    sDbg := 'GetChannelProgId /control/epg?' + ChannelId;
    m_Trace.DBMSG(TRACE_SYNC, sDbg);

    DateSeparator   := '-';
    ShortDateFormat := 'yyyy/m/d';
    TimeSeparator   := '.';
    try
      sTmp := SendHttpCommand('/control/epg?' + ChannelId);
      if Length(sTmp) > 0 then
      begin
        repeat
          sLine    := Copy(sTmp, 1, Pos(#10,sTmp)-1);
          m_Trace.DBMSG(TRACE_SYNC, sLine);
          sTmp     := Copy(sTmp, Pos(#10,sTmp)+1, Length(sTmp));
          ////////
          sEventId := IntToHex(StrToInt64Ex(Copy(sLine, 1, Pos(' ',sLine)-1)),10);

          if StrToInt64Ex(Copy(sLine, 1, Pos(' ',sLine)-1)) = 0 then
          begin
            m_Trace.DBMSG(TRACE_CALLSTACK, '< GetChannelProg');
            exit;
          end;
          if sFstEvId = '' then
          begin
            sFstEvId := sEventId;
          end;
          sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
          sZeit    := Copy(sLine, 1, Pos(' ',sLine)-1);
          ////////
          Datum := Now();
          try
            Datum  := UnixToDateTime(StrToInt64Ex(sZeit)) + OffsetFromUTC;
          except
          end;
          DateTimeToString(sDatum, 'dd.mm.yyyy', Datum);
          DateTimeToString(sUhrzeit, 'hh:nn', Datum);
          ////////
          sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
          sDauer   := Trim(Copy(sLine, 1, Pos(' ',sLine)-1));
          ////////
          sTitel   := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
          sEpg     := SaveEpgText(IntToHex(StrToInt64Ex(sEventId),10), '', sTitel, sSubTitel).Text;
          sDbg := 'GetChannelProgId eventid=' + sEventId + ' Titel=' + sTitel + ' SubTitel=' + sSubTitel;
          m_Trace.DBMSG(TRACE_SYNC, sDbg);
          DoEvents;
          ////////
          UpdateEventInDb(ChannelId,
                          sEventId,
                          sZeit,
                          sDauer,
                          sTitel,
                          sSubTitel,
                          sEpg);
          DoEvents;
        until( Length(sTmp) <= 0 );
        DoEvents;
      end;
    finally
      FillDbEventListView(ChannelId);
      GetCurrentEvent(ChannelId);
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'GetChannelProg '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< GetChannelProg');
end;

function TfrmMain.GetCurrentEvent(ChannelId: String) : String;
var
  sTmp,
  sEventId,
  sLine    : String;
  sZeit    : String;
  sDauer   : String;
  sTitel   : String;
  sSubTitel: String;
  sEpg     : String;
begin
  m_Trace.DBMSG(TRACE_CALLSTACK, '> GetCurrentEvent('+ChannelId+')');
  Result := '0';
  try
    // get the current prog ...
    sTmp := SendHttpCommand('/control/epg?ext');
    if Length(sTmp) > 0 then
    begin
      if Pos(ChannelId, sTmp)>1 then
      begin
        sTmp := Copy(sTmp, Pos(ChannelId, sTmp), Length(sTmp));
        if Length(sTmp) > 0 then
        begin
          sLine := Copy(sTmp, 1, Pos(#10,sTmp)-1);
          if sLine <> '' then
          begin
            m_Trace.DBMSG(TRACE_SYNC, sLine);
            sTmp     := Copy(sLine, 1, Pos(' ',sLine)-1);
            sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
            sZeit    := Copy(sLine, 1, Pos(' ',sLine)-1);
            sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
            sDauer   := Trim(Copy(sLine, 1, Pos(' ',sLine)-1));
            sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
            sEventId := Copy(sLine, 1, Pos(' ',sLine)-1);
            sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
            sTitel   := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
//            sEpg     := SaveEpgText(IntToHex(StrToInt64Def(sEventId,0),10), '', sTitel, sSubTitel).Text;

            lvChannelProg.Selected:= lvChannelProg.FindCaption(1,sEventId,false,true,true);
            Result := sEventId;
          end;
        end;
      end;
    end else
    begin
      sEventId := GetCurrentDbEvent(ChannelId);
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'GetCurrentEvent '+E.Message);
  end;
  DoEvents;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< GetCurrentEvent = "' + sEventId +'"');
end;

procedure TfrmMain.CheckChannelProg(ChannelId: String; sChannelName: String);
var
  sDbg     : String;
  sTmp     : String;
  sLabel   : String;
  iLabel   : Integer;
  sLine    : String;
  sEventId : String;
  sFstEvId : String;
  sZeit    : String;
  sDatum   : String;
  sUhrzeit : String;
  sDauer   : String;
  sTitel   : String;
  sSubTitel: String;
  sEpg     : String;
  sTimerStr: String;
  Datum    : TDateTime;
begin
  m_Trace.DBMSG(TRACE_CALLSTACK, '> CheckChannelProg');
  try
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannelProg (no PING)');
      exit;
    end;

    if IsDBoxRecording then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannelProg (DBox is recording now)');
      exit;
    end;

    sLabel := lbl_check_EPG.Caption;
    iLabel := 1;

    sTmp := SendHttpCommand('/control/epg?' + ChannelId);
    if Length(sTmp) > 0 then
    begin
      sTmp := sTmp + #10;
      repeat
        sDbg := 'CheckChannelProg /control/epg?' + ChannelId;
        m_Trace.DBMSG(TRACE_SYNC, sDbg);

        sLine := Copy(sTmp, 1, Pos(#10,sTmp)-1);
        if sLine = '' then
        begin
          m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannelProg (line is empty)');
          exit;
        end;
        m_Trace.DBMSG(TRACE_SYNC, sLine);

        case iLabel of
          1: begin
               iLabel:= 2;
               lbl_check_EPG.Caption:= sLabel + ' .';
             end;
          2: begin
               iLabel:= 3;
               lbl_check_EPG.Caption:= sLabel + ' ..';
             end;
          3: begin
               iLabel:= 1;
               lbl_check_EPG.Caption:= sLabel + ' ...';
             end;
        end;
        DoEvents;

        sTmp := Copy(sTmp, Pos(#10,sTmp)+1, Length(sTmp));
        ////////
        try
          sEventId := Copy(sLine, 1, Pos(' ',sLine)-1);
        except
        end;
        if sFstEvId = '' then
        begin
          sFstEvId := sEventId;
        end;
        sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
        sZeit    := Copy(sLine, 1, Pos(' ',sLine)-1);
        ////////
        Datum := Now();
        try
          Datum  := UnixToDateTime(StrToInt64Def(sZeit,0)) + OffsetFromUTC;
        except
        end;
        if Datum >= Now() then
        begin
          DateTimeToString(sDatum, 'dd.mm.yyyy', Datum);
          DateTimeToString(sUhrzeit, 'hh:nn', Datum);
          ////////
          sLine    := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
          sDauer   := IntToStr(StrToIntDef(Trim(Copy(sLine, 1, Pos(' ',sLine)-1)),60));
          ////////
          sTitel   := Copy(sLine, Pos(' ',sLine)+1, Length(sLine));
          sEpg     := SaveEpgText(IntToHex(StrToInt64Ex(sEventId),10), '', sTitel, sSubTitel).Text;
          ////////

          UpdateEventInDb(ChannelId,
                          sEventId,
                          sZeit,
                          sDauer,
                          sTitel,
                          sSubTitel,
                          sEpg);
          DoEvents;
        end;
        if IsDBoxRecording then
        begin
          m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannelProg (DBox is recording now)');
          exit;
        end;
        DoEvents;
      until( Length(sTmp) <= 0 );
      DoEvents;
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'CheckChannelProg '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannelProg');
end;

procedure TfrmMain.CheckChannels(bSwitchChannels: Boolean);
var
  ChannelId        : String;
  CurrentChannelId : String;
  i,j,x            : Integer;
  sLabel,
  sTmp             : String;
begin
  try
    m_Trace.DBMSG(TRACE_CALLSTACK, '> CheckChannels');

    if IsDBoxRecording then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannels (DBox is recording)');
      exit;
    end;

    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannels');
      exit;
    end else
    begin

      m_Trace.DBMSG(TRACE_SYNC, 'CheckChannels');
      try
        lbl_check_EPG.Caption:= 'alte Eintr�ge werden aus der Datenbank gel�scht';
        DoEvents;
        TruncDbEvent;
        DoEvents;
      except
        on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'CheckChannels '+E.Message);
      end;

      Whishes_Result.Items.Clear;
      Whishes_Result.Items.Add('Speichern aller verf�gbaren EPGs der einzelnen Sender in der Datenbank ...');
      DoEvents;
      if bSwitchChannels then
      begin
        Whishes_Result.Items.Add('- Durchschalten der vorgemerkten Sender zum Lesen der EPGs vom SAT');
        DoEvents;

        CurrentChannelId := GetDbChannelId(lbxChannelList.Items.Strings[lbxChannelList.ItemIndex]);
        lbl_check_EPG.Caption:= 'aktueller Kanal "' + GetDbChannelName(CurrentChannelId) + '"';
        DoEvents;

        try
          j:= Pred(lbxChannelList.Items.Count);
          for i := 0 to j do
          begin
            try
              ChannelId := GetDbChannelId(lbxChannelList.Items.Strings[i]);
              if GetDbChannelSwitchFlag(ChannelId) > 0 then
              begin
                m_Trace.DBMSG(TRACE_SYNC, 'Kanalwechsel zu "'+lbxChannelList.Items.Strings[i]+'"');
                sLabel:= 'Kanalwechsel zu "' + GetDbChannelName(ChannelId) + '"';
                DoEvents;
                SendHttpCommand('/fb/switch.dbox2?zapto=' + ChannelId );
                for x:= 1 to 10 do
                begin
                  DoEvents;
                  try
                    if IsDBoxRecording then
                    begin
                      m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannels (DBox is recording)');
                      exit;
                    end;
                    case x of
                      1: lbl_check_EPG.Caption:= sLabel + ' .';
                      2: lbl_check_EPG.Caption:= sLabel + ' ..';
                      3: lbl_check_EPG.Caption:= sLabel + ' ...';
                      4: lbl_check_EPG.Caption:= sLabel + ' ....';
                      5: lbl_check_EPG.Caption:= sLabel + ' .....';
                      6: lbl_check_EPG.Caption:= sLabel + ' ......';
                      7: lbl_check_EPG.Caption:= sLabel + ' .......';
                      8: lbl_check_EPG.Caption:= sLabel + ' ........';
                      9: lbl_check_EPG.Caption:= sLabel + ' .........';
                     10: lbl_check_EPG.Caption:= sLabel + ' ..........';
                     else
                       lbl_check_EPG.Caption:= sLabel + ' ';
                    end;
                  except
                  end;
                  DoEvents;
                  Sleep(1000);
                end;
              end;
            except
            end;
            DoEvents;
          end;
        finally
          lbl_check_EPG.Caption:= 'Wechsel zur�ck zu  "' + GetDbChannelName(CurrentChannelId) + '"';
          DoEvents;
          SendHttpCommand('/fb/switch.dbox2?zapto=' + CurrentChannelId );
        end;
      end;

      Whishes_Result.Items.Add('- Lesen und Speichern der EPGs ...');
      DoEvents;
      j:= Pred(lbxChannelList.Items.Count);
      for i := 0 to j do
      begin
        try
          ChannelId := GetDbChannelId(lbxChannelList.Items.Strings[i]);
          m_Trace.DBMSG(TRACE_SYNC, 'Check Channel "'+lbxChannelList.Items.Strings[i]+'"');
          if GetDbChannelEpgFlag( ChannelId ) > 0 then
          begin
            lbl_check_EPG.Caption:= 'lese EPGs von "' + lbxChannelList.Items.Strings[i] + '"';
            Whishes_Result.Items.Add('  '+lbxChannelList.Items.Strings[i]);
            DoEvents;
            CheckChannelProg(ChannelId, sTmp);
            if IsDBoxRecording then
            begin
              m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannels (DBox is recording)');
              exit;
            end;
          end;
        except
        end;
        DoEvents;
      end;
      Whishes_Result.Items.Add('... fertig');
      m_LastEpgUpdate := Now;
      SaveSettings;
    end;

  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'CheckChannels '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< CheckChannels');
end;

procedure TfrmMain.RetrieveChannelList(ChannelList : TListBox);
var
  TmpList : TStringList;
  ChannelId : String;
  i       : Integer;
  sTmp    : String;
begin
  try
    m_Trace.DBMSG(TRACE_CALLSTACK, '> RetrieveChannelList');
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< RetrieveChannelList');
      exit;
    end else
    begin
      m_Trace.DBMSG(TRACE_DETAIL, 'RetrievChannelList');
      TmpList := TStringList.Create;
      ChannelList.Items.BeginUpdate;
      ChannelList.Items.Clear;
      try
        sTmp := SendHttpCommand('/control/channellist');
        if Length(sTmp) > 0 then
        begin
          m_Trace.DBMSG(TRACE_SYNC, sTmp);
          TmpList.Text := sTmp;
          for i := 0 to Pred(TmpList.Count) do
          begin
            sTmp := Copy(TmpList.Strings[i] , 1, Pos(' ', TmpList.Strings[i])-1);
            try
              ChannelId := sTmp;
              sTmp := Copy(TmpList.Strings[i] , Pos(' ', TmpList.Strings[i])+1, Length(TmpList.Strings[i]));
              ChannelList.Items.Add(sTmp);
              UpdateChannelInDb(ChannelId, sTmp);
            except
            end;
            DoEvents;
          end;
          RetrieveCurrentChannel(ChannelList, sTmp);
        end else
        begin
          FillDbChannelListBox;
        end;
      finally
        FreeAndNil(TmpList);
        ChannelList.Items.EndUpdate;
      end;
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'RetrieveChannelList '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< RetrieveChannelList');
end;

function  TfrmMain.RetrieveCurrentChannel(ChannelList : TListBox; var channelname: String): String;
var
  Channel_Id,
  chidx      : String;
  i,x        : Integer;
  sTmp       : String;
begin
  Result := '0';
  try
    m_Trace.DBMSG(TRACE_CALLSTACK, '> RetrieveCurrentChannel');
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< RetrieveCurrentChannel');
      exit;
    end else
    begin
      m_Trace.DBMSG(TRACE_DETAIL, 'RetrieveCurrentChannel');
      sTmp := SendHttpCommand('/control/zapto');
      if Length(sTmp) > 0 then
      begin
        sTmp := Copy(sTmp, 1, Pos(#10, sTmp)-1);
        Channel_Id := Trim(sTmp);
        Result  := Channel_Id;
        for i := 0 to Pred(ChannelList.Items.Count) do
        begin
          chidx := GetDbChannelId(lbxChannelList.Items.Strings[i]);
          if chidx = Channel_Id then
          begin
            ChannelList.ItemIndex := i;
            m_Trace.DBMSG(TRACE_DETAIL, 'Current Channel is '+ChannelList.Items.Strings[i]);
            channelname := ChannelList.Items.Strings[i];
            FillDbEventListView(Channel_Id);
            DoEvents;
            break;
          end;
        end;
      end;
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'RetrieveChannelList '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< RetrieveCurrentChannel');
end;

procedure TfrmMain.InitChannels;
begin
  try
    m_Trace.DBMSG(TRACE_CALLSTACK, '> InitChannels');
    if not PingDBox(m_sDBoxIp) then
    begin
      FillDbChannelListBox;
      m_Trace.DBMSG(TRACE_CALLSTACK, '< InitChannels');
      exit;
    end else
    begin
      RetrieveChannelList( lbxChannelList );
      if (lbxChannelList.ItemIndex < 0) and (lbxChannelList.Count > 0) then
      begin
        lbxChannelList.ItemIndex := 0;
      end;
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR,'InitChannels '+ E.Message );
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< InitChannels');
end;

procedure TfrmMain.GetPids(var vpid: Integer; var apids: array of Integer);
var
  sTmp,
  sLine : String;
begin
  try
    m_Trace.DBMSG(TRACE_CALLSTACK, '> GetPids');
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< GetPids');
      exit;
    end;
    sTmp := SendHttpCommand('/control/zapto?getpids');
    if Length(sTmp) > 0 then
    begin
      m_Trace.DBMSG(TRACE_DETAIL, '/control/zapto?getallpids >> ' + sTmp);
      sLine := Copy(sTmp, 1, Pos(#10,sTmp)-1);
      vpid:= StrToIntEx(sLine);
      m_Trace.DBMSG(TRACE_DETAIL, sLine);
      sTmp  := Copy(sTmp, Pos(#10,sTmp)+1, Length(sTmp));
      sLine := Copy(sTmp, 1, Pos(#10,sTmp)-1);
      apids[0]:= StrToIntEx(sLine);
      m_Trace.DBMSG(TRACE_DETAIL, sLine);
      sTmp  := Copy(sTmp, Pos(#10,sTmp)+1, Length(sTmp));
      if Length(sTmp) > 0 then
      begin
        sLine := Copy(sTmp, 1, Pos(#10,sTmp)-1);
        apids[1]:= StrToIntEx(sLine);
        m_Trace.DBMSG(TRACE_DETAIL, sLine);
        sTmp  := Copy(sTmp, Pos(#10,sTmp)+1, Length(sTmp));
        if Length(sTmp) > 0 then
        begin
          sLine := Copy(sTmp, 1, Pos(#10,sTmp)-1);
          apids[2]:= StrToIntEx(sLine);
          m_Trace.DBMSG(TRACE_DETAIL, sLine);
        end;
      end;
    end;

  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'GetPids '+E.Message);
  end;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< GetPids');
end;

procedure TfrmMain.SendSectionsPauseCommand(aPause: Boolean);
begin
  try
    m_Trace.DBMSG(TRACE_CALLSTACK, '> SendSectionsPauseCommand');
    if not PingDBox(m_sDBoxIp) then
    begin
      m_Trace.DBMSG(TRACE_CALLSTACK, '< SendSectionsPauseCommand');
      exit;
    end;
    if aPause then
    begin
      if chxKillSectionsd.Checked then SendHttpCommand('/control/zapto?stopsectionsd');
    end else
    begin
      SendHttpCommand('/control/zapto?startsectionsd');
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'SendSectionsPauseCommand '+E.Message);
  end;
  DoEvents;
  m_Trace.DBMSG(TRACE_CALLSTACK, '< SendSectionsPauseCommand');
end;

////////////////////////////////////////////////////////////////////////////////

procedure TfrmMain.VCR_PingReply(ASender: TComponent;
                                 const AReplyStatus: TReplyStatus);
begin
  if AReplyStatus.BytesReceived > 0 then
  begin
    m_bVCR_Ping_Ok := true;
  end else
  begin
    m_bVCR_Ping_Ok := false;
  end;
  m_Trace.DBMSG(TRACE_SYNC, 'PingReply '+ IntToStr(AReplyStatus.BytesReceived) );
end;

function  TfrmMain.PingDBox(sIp: String) : Boolean;
begin
  Result:= false;
  try
    m_Trace.DBMSG(TRACE_SYNC, 'PingDBox '+ sIp );
    VCR_Ping.Host := sIp;
    m_bVCR_Ping_Ok := false;
    VCR_Ping.Ping;
    Application.ProcessMessages;
    Sleep(100);
    Application.ProcessMessages;
    Result := m_bVCR_Ping_Ok;
    if m_bVCR_Ping_Ok then
    begin
      m_Trace.DBMSG(TRACE_SYNC, 'PingDBox '+ sIp + ' = true' );
    end else
    begin
      m_Trace.DBMSG(TRACE_SYNC, 'PingDBox '+ sIp + ' = false' );
    end;
  except
    on E: Exception do m_Trace.DBMSG(TRACE_ERROR, 'PingDBox '+ E.Message );
  end;
end;

//
//  HTTP-Clients to receive Infos from DBox ...
//
////////////////////////////////////////////////////////////////////////////////